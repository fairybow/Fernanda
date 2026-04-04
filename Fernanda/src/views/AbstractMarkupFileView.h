/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <utility>

#include <QEvent>
#include <QHBoxLayout>
#include <QObject>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>

#include "core/Time.h"
#include "models/TextFileModel.h"
#include "ui/WidgetSnapshotOverlay.h"
#include "views/MarkupPreviewPage.h"
#include "views/MarkupWebcode.h"
#include "views/TextFileView.h"

namespace Fernanda {

/// TODO MU: I'd maybe like a 3-way toggle switch instead of cycling labels and
/// functionality
/// TODO MU: Scroll lock
/// TODO MU: Preview auto-scroll for new content added (like a soft scroll lock
/// while typing at the end of the page)
class AbstractMarkupFileView : public TextFileView
{
    Q_OBJECT

public:
    enum Mode
    {
        Split = 0,
        Edit,
        Preview
    };

    explicit AbstractMarkupFileView(
        TextFileModel* fileModel,
        QWidget* parent = nullptr,
        int reparseDebounce =
            5) // TODO: Maybe remove as ctor arg. Both parsers can handle 0 but
               // keeping both at 5 ms saves calls and normalizes the feel of
               // typing in each view type
        : TextFileView(fileModel, parent)
        , reparseTimer_(
              Time::newDebouncer(
                  this,
                  &AbstractMarkupFileView::reparse_,
                  reparseDebounce))
    {
    }

    virtual ~AbstractMarkupFileView() override {}

    Mode mode() const noexcept { return mode_; }

    void setMode(Mode mode)
    {
        if (!splitter_ || !preview_) return;

        // During the transition, QWebEngineView will look really jank. This is
        // unavoidable. What we can do, though, is hide the transition using a
        // screenshot of the widgets' prior states while they transition to the
        // next. NB: We want to avoid calling setMode in setupWidget, otherwise,
        // we'll get a broken screengrab visible on first display
        snapshotOverlay_->captureAndShow(container_);

        mode_ = mode;
        auto editor = this->editor();

        switch (mode) {

            // Until we have icons, use space padding to keep button roughly the
            // same size

        default:
        case Split:
            // TODO: Better way to manage button icon/text state based on the
            // cycle?
            modeToggle_->setText("Preview"); /// TODO MU: Temp
            editor->show();
            preview_->show();
            splitter_->setFocusProxy(editor);
            break;

        case Preview:
            modeToggle_->setText("Edit   "); /// TODO MU: Temp
            editor->hide();
            preview_->show();
            splitter_->setFocusProxy(preview_);
            break;

        case Edit:
            modeToggle_->setText("Split  "); /// TODO MU: Temp
            editor->show();
            preview_->hide();
            splitter_->setFocusProxy(editor);
            break;
        }

        if (mode != Edit) reparse_();

        // We need a value that is short enough to not be ridiculous but long
        // enough to cover us in case the web document is large...
        // NB: This doesn't really work in debug (only release), because
        // QWebEngineView takes too long to load
        Time::delay(250, this, [this] { snapshotOverlay_->hideOverlay(); });
    }

    void cycleMode()
    {
        switch (mode_) {

        default:
        case Split:
            setMode(Preview);
            break;

        case Preview:
            setMode(Edit);
            break;

        case Edit:
            setMode(Split);
            break;
        }
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == preview_ && event->type() == QEvent::Resize) {
            // Hide preview resize visual stutter and debounce
            previewMask_->setFixedSize(preview_->size());
            previewMask_->raise();
            previewMask_->show();
            previewMaskTimer_->start();
        }

        return TextFileView::eventFilter(watched, event);
    }

protected:
    virtual QWidget* setupWidget() override
    {
        modeBar_->setFixedHeight(28);

        // Since we start in split (handle this better/dynamically without
        // calling setMode, eventually):
        modeToggle_->setText("Preview"); /// TODO MU: Temp
        modeToggle_->setFixedHeight(22);

        auto editor_widget = TextFileView::setupWidget();
        editor_widget->setMinimumWidth(MIN_WIDGET_SIZE_);

        preview_->setPage(new MarkupPreviewPage(preview_));
        preview_->setMinimumWidth(MIN_WIDGET_SIZE_);
        previewMask_->setAutoFillBackground(true);
        previewMask_->hide();

        splitter_->addWidget(editor_widget);
        splitter_->addWidget(preview_);

        splitter_->setStretchFactor(0, 0);
        splitter_->setStretchFactor(1, 1); // Let resizing favor preview

        // This is the only way to ensure the splitter starts with the handle
        // halfway (only tested when starting in Split mode - might not work if
        // starting in Edit) (Show event might also work, if ever needed)
        Time::onNextTick(this, [this] {
            auto w = splitter_->width();
            splitter_->setSizes({ w / 2, w - w / 2 });
        });

        splitter_->setChildrenCollapsible(false);
        splitter_->setFocusProxy(editor_widget);

        // Layout
        auto mode_bar_layout = new QHBoxLayout(modeBar_);
        mode_bar_layout->setContentsMargins(2, 2, 2, 2);
        mode_bar_layout->setSpacing(0);
        mode_bar_layout->addWidget(modeToggle_, 0);
        mode_bar_layout->addStretch();

        auto container_layout = new QVBoxLayout(container_);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container_layout->addWidget(modeBar_, 0);
        container_layout->addWidget(splitter_, 1);

        connect(
            editor()->document(),
            &QTextDocument::contentsChanged,
            this,
            [this] {
                // In the contentsChanged connection:
                if (preview_ && preview_->isVisible())
                    reparseTimer_->start();
                else
                    previewStale_ = true;
            });

        connect(modeToggle_, &QToolButton::clicked, this, [this] {
            cycleMode();
        });

        // Can't call setMode to start (see setMode note)
        splitter_->setFocusProxy(editor_widget);
        reparse_();

        // QWebEngineView's first ever load will wreak visual havoc. We can't
        // use previewMask_, as it's parented to the web engine view itself (and
        // the warm-up process causes the web engine view to raise itself in a
        // way we can't fight). Parenting previewMask_ to the splitter causes it
        // to be *added* to the splitter (no good); parenting it to `this`
        // causes it to raise above the snapshot overlay on mode change (no
        // good); and there's no point in trying container_ or
        // reworking/replanning what already works (parenting it to preview_ and
        // letting it do *that* job of covering preview_ on resize and initial
        // (regular) load. So, we'll make a separate mask for this specific
        // problem
        if (firstEverLoad_) {
            warmupMask_ = new QWidget(this);
            warmupMask_->setAutoFillBackground(true);
            warmupMask_->raise();
            warmupMask_->show();

            connect(
                preview_->page(),
                &QWebEnginePage::loadFinished,
                this,
                [this] {
                    Time::delay(250, this, [this] {
                        if (!warmupMask_) return;
                        warmupMask_->hide();
                        warmupMask_->deleteLater();
                        warmupMask_ = nullptr;
                        firstEverLoad_ = false;
                    });
                },
                Qt::SingleShotConnection);
        }

        // Mask the preview until QWebEngineView finishes its first load
        // TODO: Watch/adjust this if we ever allow starting in a mode other
        // than Split
        previewMask_->setFixedSize(preview_->size());
        previewMask_->raise();
        previewMask_->show();

        connect(
            preview_->page(),
            &QWebEnginePage::loadFinished,
            this,
            [this] {
                Time::onNextTick(this, [this] { previewMask_->hide(); });
            },
            Qt::SingleShotConnection);

        preview_->installEventFilter(this);

        return container_;
    }

    // Subclasses implement these to convert plain text to HTML for the preview
    // (see `reparse_` note):

    virtual QStringView css() const = 0;
    virtual QStringList htmlBlocks(const QString& plainText) const = 0;

    /// TODO MU: Consider restructuring Fountain CSS and removing. These are
    /// kind of just suppoting a holdover (article/section tags) from original's
    /// CSS
    virtual QString bodyPrefix() const { return {}; }
    virtual QString bodySuffix() const { return {}; }

    virtual void showEvent(QShowEvent* event)
    {
        TextFileView::showEvent(event);

        if (previewStale_ && preview_->isVisible()) {
            previewStale_ = false;
            reparse_();
        }
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        TextFileView::resizeEvent(event);
        if (warmupMask_) warmupMask_->setFixedSize(size());
    }

    // QWebEngineView* preview() const noexcept { return preview_; }

private:
    Time::Debouncer* reparseTimer_;

    constexpr static int MIN_WIDGET_SIZE_ = 50;
    bool firstParse_ = true;
    bool previewStale_ = false;
    QStringList cachedBlocks_{};

    Mode mode_ = Split;
    QWidget* container_ = new QWidget(this);
    WidgetSnapshotOverlay* snapshotOverlay_ = new WidgetSnapshotOverlay(this);
    QWidget* modeBar_ = new QWidget(this);
    QToolButton* modeToggle_ = new QToolButton(this);
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    QWebEngineView* preview_ = new QWebEngineView(this);
    QWidget* previewMask_ = new QWidget(preview_);
    Time::Debouncer* previewMaskTimer_ =
        Time::newDebouncer(this, [this] { previewMask_->hide(); }, 300);

    inline static bool firstEverLoad_ = true;
    QWidget* warmupMask_ = nullptr;

    static QString appFontFaceKit_();

    // Incremental DOM patching
    //
    // Subclasses return a QStringList from htmlBlocks() where each entry is one
    // top-level HTML element with a data-idx='N' attribute (N matching its list
    // index). On subsequent reparses, we diff the new list against the cached
    // one and patch only changed blocks via JS outerHTML assignment. When the
    // block count changes (insertions/deletions), we fall back to full
    // innerHTML replacement
    //
    // Subclasses that wrap their output in container elements (article,
    // section, etc.) should return those via bodyPrefix()/bodySuffix() rather
    // than including them in the block list, since they are not indexed and are
    // only used for first parse and full replacement
    /// TODO MU: Print total output for this to check md/fn
    void reparse_()
    {
        auto editor = this->editor();
        if (!preview_ || !editor) return;

        auto blocks = htmlBlocks(editor->document()->toPlainText());

        if (firstParse_) {
            firstParse_ = false;

            auto body = bodyPrefix() + blocks.join(QString{}) + bodySuffix();
            auto html = MarkupWebcode::htmlDoc(appFontFaceKit_(), css(), body);

            /// TODO MU: I am vaguely concerned about the baseUrl
            preview_->setHtml(html, QUrl("qrc:/"));
            cachedBlocks_ = std::move(blocks);
            return;
        }

        // Try incremental patch when block count is unchanged
        if (blocks.size() == cachedBlocks_.size()) {
            QString js_patch{};

            for (auto i = 0; i < blocks.size(); ++i) {
                if (blocks[i] == cachedBlocks_[i]) continue;

                QString escaped = blocks[i];
                escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
                escaped.replace(QStringLiteral("`"), QStringLiteral("\\`"));

                js_patch += MarkupWebcode::jsOuterHtml(i, escaped);
            }

            if (!js_patch.isEmpty()) {
                preview_->page()->runJavaScript(
                    MarkupWebcode::jsPatchHtmlBody(js_patch));
            }

            cachedBlocks_ = std::move(blocks);
            return;
        }

        // Full fallback replacement (block count changed)
        auto body = bodyPrefix() + blocks.join(QString()) + bodySuffix();
        body.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        body.replace(QStringLiteral("`"), QStringLiteral("\\`"));

        preview_->page()->runJavaScript(MarkupWebcode::jsReplaceHtmlBody(body));
        cachedBlocks_ = std::move(blocks);
    }
};

} // namespace Fernanda
