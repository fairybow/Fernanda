/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QHBoxLayout>
#include <QShowEvent>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include "core/BundledFonts.h"
#include "core/Time.h"
#include "core/Tr.h"
#include "models/TextFileModel.h"
#include "ui/MultiSwitch.h"
#include "ui/WidgetMask.h"
#include "ui/WidgetSnapshotOverlay.h"
#include "views/MarkupWebcode.h"
#include "views/TextFileView.h"
#include "views/WebEnginePage.h"
#include "views/WebEngineView.h"

namespace Fernanda {

/// TODO MU: Scroll lock
/// TODO MU: Preview auto-scroll for new content added (like a soft scroll lock
/// while typing at the end of the page)
class AbstractMarkupFileView : public TextFileView
{
    Q_OBJECT

public:
    enum Mode
    {
        Edit,
        Split,
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

    Mode mode() const noexcept { return mode_; }

    void setMode(Mode mode)
    {
        modeSwitch_->setIndex(static_cast<int>(mode));
        applyMode_(mode);
    }

protected:
    virtual QWidget* setupWidget() override
    {
        auto editor_widget = TextFileView::setupWidget();
        editor_widget->setMinimumWidth(MIN_WIDGET_SIZE_);

        preview_->setPage(new WebEnginePage(preview_));
        preview_->setMinimumWidth(MIN_WIDGET_SIZE_);

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
        mode_bar_layout->setContentsMargins(5, 5, 5, 5);
        mode_bar_layout->setSpacing(0);
        mode_bar_layout->addStretch();
        mode_bar_layout->addWidget(modeSwitch_, 0);
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
                if (preview_ && preview_->isVisible()) {
                    reparseTimer_->start();
                } else {
                    previewStale_ = true;
                }
            });

        connect(
            modeSwitch_,
            &MultiSwitch::indexChanged,
            this,
            [this](int index) { applyMode_(static_cast<Mode>(index)); });

        // Can't call setMode to start (see setMode note)
        splitter_->setFocusProxy(editor_widget);
        reparse_();

        // QWebEngineView's first-ever load causes visual havoc. The preview's
        // own mask can't help here (the web engine raises itself during
        // warm-up). A separate mask parented to `this` covers the whole view
        // until the first load completes
        if (WebEngineView::firstEverLoad()) {
            auto warmup_mask = new WidgetMask(this, 0);
            warmup_mask->activate(true);

            connect(
                preview_->page(),
                &QWebEnginePage::loadFinished,
                this,
                [this, warmup_mask] {
                    Time::delay(250, this, [warmup_mask] {
                        warmup_mask->deactivate(true);
                    });
                },
                Qt::SingleShotConnection);
        }

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

    virtual void showEvent(QShowEvent* event) override
    {
        TextFileView::showEvent(event);

        if (previewStale_ && preview_->isVisible()) {
            previewStale_ = false;
            reparse_();
        }
    }

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
    MultiSwitch* modeSwitch_ = new MultiSwitch(
        { Tr::previewEdit(), Tr::previewSplit(), Tr::previewPreview() },
        1,
        this);
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    WebEngineView* preview_ = new WebEngineView(this);

    void applyMode_(Mode mode)
    {
        if (mode == mode_) return;

        // During the transition, QWebEngineView will look really jank. This is
        // unavoidable. What we can do, though, is hide the transition using a
        // screenshot of the widgets' prior states while they transition to the
        // next. NB: We want to avoid calling setMode in setupWidget, otherwise,
        // we'll get a broken screengrab visible on first display
        snapshotOverlay_->captureAndShow(container_);

        mode_ = mode;
        auto editor = this->editor();

        switch (mode) {

        default:
        case Split:
            editor->show();
            preview_->show();
            splitter_->setFocusProxy(editor);
            break;

        case Preview:
            editor->hide();
            preview_->show();
            splitter_->setFocusProxy(preview_);
            break;

        case Edit:
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
            auto html =
                MarkupWebcode::htmlDoc(BundledFonts::cssAtRules(), css(), body);

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
                escaped.replace(u"\\"_s, u"\\\\"_s);
                escaped.replace(u"`"_s, u"\\`"_s);

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
        body.replace(u"\\"_s, u"\\\\"_s);
        body.replace(u"`"_s, u"\\`"_s);

        preview_->page()->runJavaScript(MarkupWebcode::jsReplaceHtmlBody(body));
        cachedBlocks_ = std::move(blocks);
    }
};

} // namespace Fernanda
