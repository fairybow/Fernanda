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

#include <QEvent>
#include <QHBoxLayout>
#include <QObject>
#include <QShowEvent>
#include <QSplitter>
#include <QString>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>

#include "core/Time.h"
#include "models/TextFileModel.h"
#include "ui/WidgetSnapshotOverlay.h"
#include "views/MarkupPreviewPage.h"
#include "views/TextFileView.h"

namespace Fernanda {

/// TODO MU: I'd maybe like a 3-way toggle switch instead of cycling labels and
/// functionality
/// TODO MU: Still a little flicker on first show for preview
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
        int reparseDebounce,
        QWidget* parent = nullptr)
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
    /// TODO MU: Use resize mask code above (separate into own method) to hide
    /// during startup. Should be fine now that it's just an overlay. However,
    /// the same time we use for resizing might not be right for this...
    virtual QWidget* setupWidget() override
    {
        modeBar_->setFixedHeight(24);

        // Since we start in split (handle this better/dynamically without
        // calling setMode, eventually):
        modeToggle_->setText("Preview"); /// TODO MU: Temp
        modeToggle_->setFixedHeight(20);

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

        preview_->installEventFilter(this);

        return container_;
    }

    // Subclasses implement this to convert plain text to HTML for the preview
    virtual QString renderToHtml(const QString& plainText) const = 0;

    virtual void showEvent(QShowEvent* event)
    {
        TextFileView::showEvent(event);
        if (previewStale_ && preview_->isVisible()) {
            previewStale_ = false;
            reparse_();
        }
    }

    QWebEngineView* preview() const noexcept { return preview_; }

private:
    Time::Debouncer* reparseTimer_;

    constexpr static int MIN_WIDGET_SIZE_ = 50;
    bool firstParse_ = true;
    bool previewStale_ = false;

    Mode mode_ = Split;
    QWidget* container_ = new QWidget(this);
    WidgetSnapshotOverlay* snapshotOverlay_ = new WidgetSnapshotOverlay(this);
    QWidget* modeBar_ = new QWidget(this);
    QToolButton* modeToggle_ = new QToolButton(this);
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    QWebEngineView* preview_ = new QWebEngineView(this);
    QWidget* previewMask_ = new QWidget(preview_);
    Time::Debouncer* previewMaskTimer_ =
        Time::newDebouncer(this, [this] { previewMask_->hide(); }, 250);

    static QString appFontFaceKit_();

    /// TODO MU: BUG: Fountain seems to not return to correct scroll position.
    /// Seems like sometimes it works, sometimes it doesn't, which just means
    /// it's some random action causing it that I haven't identified yet. May or
    /// may not also be the case for Markdown (don't see why it wouldn't be) but
    /// haven't tested it as much
    void reparse_()
    {
        auto editor = this->editor();
        if (!preview_ || !editor) return;

        auto html = renderToHtml(editor->document()->toPlainText());

        if (firstParse_) {
            firstParse_ = false;

            html.replace(
                QStringLiteral("</head>"),
                QStringLiteral("<style>%1</style></head>")
                    .arg(appFontFaceKit_()));
            preview_->setHtml(
                html,
                QUrl("qrc:/")); /// TODO MU: I am vaguely concerned about this

            return;
        }

        // Extract just the body content and swap it via DOM manipulation to
        // avoid flickering
        auto body_start = html.indexOf(QStringLiteral("<body>"));
        auto body_end = html.indexOf(QStringLiteral("</body>"));

        if (body_start < 0 || body_end < 0) {
            preview_->setHtml(html, QUrl("qrc:/")); /// TODO MU: See above
            return;
        }

        auto body_content = html.mid(body_start + 6, body_end - body_start - 6);

        // Escape for JS string
        body_content.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        body_content.replace(QStringLiteral("`"), QStringLiteral("\\`"));

        preview_->page()->runJavaScript(
            QStringLiteral(
                "var scrollY = window.scrollY; document.body.innerHTML = `%1`; "
                "window.scrollTo(0, scrollY);")
                .arg(body_content));
    }
};

} // namespace Fernanda
