/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QJsonObject>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/VaultConfig.h"
#include "core/spell/SpellChecker.h"
#include "core/spell/WordList.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

// One view of one file. A QWidget base so file types are handled
// polymorphically (text, PDF, image), which is why TextFileView holds a
// QPlainTextEdit as a child rather than subclassing it.
//
// Construction: the derived constructor builds its own content widget and hands
// it to setWidget(). A derived ctor body runs after the base is fully
// constructed, so there's no dispatch-to-base problem and no second-phase call
// to forget.
//
// Borrows its model from the Vault; never owns it. Holds no path of its own —
// model()->key() is the single source.
//
// Built parentless: every view is made by VaultWindow::makeView_ and handed to
// a TabPaneLeaf, whose page stack parents it on add. So there is no parent
// parameter at all — it would only ever be nullptr (house rule: no default
// args for arguments that are never passed).
//
// Load failure. A view whose file couldn't be loaded — a corrupt or locked PDF,
// an image Qt can't decode (a damaged file, or a missing TIFF/WebP imageformats
// plugin) — swaps its content widget for a centered message via showLoadFailure
// / showLoadedContent. Lives here rather than in each view because PDF and
// image share it exactly; text never calls it and pays nothing (the label is
// created on first failure). Both widgets sit in the base's layout and one is
// hidden — a hidden widget takes no space, so no QStackedWidget is needed
class AbstractFileView : public QWidget
{
    Q_OBJECT

public:
    explicit AbstractFileView(AbstractFileModel* fileModel)
        : QWidget(nullptr)
        , model_(fileModel)
    {
        ASSERT(fileModel, "AbstractFileView requires a model!");
        setup_();
    }

    ~AbstractFileView() override = default;

    [[nodiscard]] AbstractFileModel* model() const noexcept { return model_; }

    // Per-view zoom, dispatched to by the window's view.zoom* actions. No-ops
    // on the base by design — the same inert-default shape as the model's
    // undo/redo: a view that can't zoom (text, an empty tab) inherits these and
    // does nothing, so the window action stays always-enabled and simply has no
    // effect there. PdfFileView and ImageFileView override them. Zoom is
    // per-view presentation state, so it lives on the view, never on the shared
    // model
    virtual void zoomIn() {}
    virtual void zoomOut() {}
    virtual void zoomReset() {}

    // Search within the file, dispatched to by the window's view.find*
    // actions. No-ops on the base, as zoom is: a view with nothing to search
    // (a PDF, an image) inherits these and does nothing. TextFileView
    // overrides them. The search is one view's own, like its zoom: two views
    // of a file search apart
    virtual void showFind() {}
    virtual void showReplace() {}
    virtual void findNext() {}
    virtual void findPrevious() {}

    // Per-view persistence into workspace.json's tab entry. Each view
    // reads/writes ONLY within the opaque "state" blob it's handed — the
    // workspace layer never inspects it (Obsidian's getState/setState shape).
    // No-ops on the base: a view with no persistent viewport state contributes
    // nothing, and an absent or empty state on restore leaves it at its
    // defaults. describePage_/buildPage_ own the envelope and nest this under
    // WorkspaceKeys::STATE. A view defers whatever needs layout (scroll, a PDF
    // page) to its own showEvent rather than applying it here
    virtual void writeViewState(QJsonObject&) const {}
    virtual void readViewState(const QJsonObject&) {}

    // Per-vault settings, applied at construction and again on every
    // Vault::configChanged — both wired by VaultWindow::makeView_, from the
    // HOSTING window's vault. So a Common Vault file open in a project window
    // takes the project vault's settings, and one open in the Common Vault's
    // own window takes the Common Vault's. A tab only ever moves within its
    // window family, all of one vault, so a view's source never changes. No-op
    // on the base, like the zoom virtuals: a view with nothing to apply (PDF,
    // image) inherits it and ignores the config
    virtual void applyConfig(const VaultConfig&) {}

    // The dictionary to check spelling against, or nullptr for no checking.
    // Set alongside applyConfig, by the same wiring, since which one it is
    // comes from the same config. The view borrows it. No-op on the base: a
    // view with no text to check ignores it
    virtual void setSpellChecker(SpellChecker*) {}

    // Words to take as correctly spelled whatever the dictionary says: the
    // vault's own, and those ignored for now. Set by the same wiring. No-op
    // on the base, like setSpellChecker
    virtual void setAcceptedWords(const WordList&) {}

protected:
    // The derived ctor calls this once with its content widget. The base owns
    // the layout and focus proxy; the widget itself is parented into the layout
    void setWidget(QWidget* widget)
    {
        ASSERT(widget, "setWidget requires a widget!");
        ASSERT(!widget_, "setWidget called twice!");

        widget_ = widget;
        layout_->addWidget(widget_);
        setFocusProxy(widget_);
    }

    // Hide the content widget and show message in its place (see class note).
    // Anything parented to the content widget — a PDF or image view's zoom
    // pill — hides with it. Called again with a new message, only the text
    // changes. Requires setWidget first, since the label goes in the layout
    // after the content
    void showLoadFailure(const QString& message)
    {
        ASSERT(widget_, "showLoadFailure called before setWidget!");

        if (!failureLabel_) {
            failureLabel_ = new QLabel(this);
            failureLabel_->setAlignment(Qt::AlignCenter);
            failureLabel_->setWordWrap(true);
            layout_->addWidget(failureLabel_);
        }

        failureLabel_->setText(message);
        widget_->hide();
        failureLabel_->show();
    }

    // The reverse: the content widget back, the message (if one was ever
    // shown) hidden. Safe to call when no failure was shown — a reload that
    // finds a now-valid file calls this unconditionally
    void showLoadedContent()
    {
        ASSERT(widget_, "showLoadedContent called before setWidget!");

        if (failureLabel_) {
            failureLabel_->hide();
        }

        widget_->show();
    }

private:
    AbstractFileModel* model_;
    QVBoxLayout* layout_ = nullptr;
    QWidget* widget_ = nullptr;
    QLabel* failureLabel_ = nullptr; // created on first showLoadFailure

    void setup_()
    {
        // Count this view against the model, so the Vault can evict the buffer
        // when the last view closes. The model watches our destroyed and never
        // dereferences us, so this stays GUI-free on the model side. Exactly
        // one registration per view — this is the single call site
        model_->addView(this);

        layout_ = new QVBoxLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setSpacing(0);

        // Self-close if the file is deleted on disk while open. The Vault's
        // watcher fires removedFromDisk, then frees the model; every view on
        // that model (this window's pop-outs, or — for a common-vault file —
        // any window) drops itself. deleteLater is submitted before the Vault
        // frees the model, so the view tears down first and never touches a
        // dead model; the leaf hosting this page removes its tab as the page is
        // destroyed
        connect(
            model_,
            &AbstractFileModel::removedFromDisk,
            this,
            &QObject::deleteLater);

        // The tab title is the model's title (its file stem). Mirror it onto
        // our own windowTitle and keep it current — the hosting leaf watches
        // windowTitleChanged to retitle the tab, so it stays
        // page-type-ignorant. A rename fans out from the shared model, so every
        // window showing this file retitles. windowTitle only re-emits on an
        // actual change, so an unchanged stem (a folder rename) is a no-op here
        setWindowTitle(model_->title());
        connect(model_, &AbstractFileModel::renamed, this, [this] {
            setWindowTitle(model_->title());
        });
    }
};

} // namespace Fernanda
