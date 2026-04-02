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

#include <QHBoxLayout>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedWidget> ///
#include <QString>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>

#include "core/Time.h"
#include "models/TextFileModel.h"
#include "views/MarkupPreviewPage.h"
#include "views/TextFileView.h"

namespace Fernanda {

// TODO: Test and see if we need a loading mask over preview or entire container
// (probably the latter); if we do, can connect to web view's load start and end
// signals to swap in a stacked widget
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
        QWidget* parent = nullptr)
        : TextFileView(fileModel, parent)
    {
    }

    virtual ~AbstractMarkupFileView() override {}

    Mode mode() const noexcept { return mode_; }

    void setMode(Mode mode)
    {
        if (!splitter_ || !preview_) return;

        mode_ = mode;
        auto editor = this->editor();

        switch (mode) {

        default:
        case Split:
            editor->show();
            preview_->show();
            splitter_->setFocusProxy(editor);
            break;

        case Edit:
            editor->show();
            preview_->hide();
            splitter_->setFocusProxy(editor);
            break;

        case Preview:
            editor->hide();
            preview_->show();
            splitter_->setFocusProxy(preview_);
            break;
        }

        if (mode != Edit) reparse_();
    }

    void cycleMode()
    {
        switch (mode_) {

        default:
        case Split:
            setMode(Preview);
            break;

        case Edit:
            setMode(Split);
            break;

        case Preview:
            setMode(Edit);
            break;
        }
    }

protected:
    virtual QWidget* setupWidget() override
    {
        modeBar_->setFixedHeight(24);
        modeToggle_->setText("Mode"); /// TODO MU: Temp
        modeToggle_->setFixedHeight(20);

        auto editor_widget = TextFileView::setupWidget();
        editor_widget->setMinimumWidth(MIN_WIDGET_SIZE_);

        preview_->setPage(new MarkupPreviewPage(preview_));
        preview_->setMinimumWidth(MIN_WIDGET_SIZE_);

        splitter_->addWidget(editor_widget);
        splitter_->addWidget(preview_);

        splitter_->setStretchFactor(0, 0);
        splitter_->setStretchFactor(1, 1); // Let resizing favor preview

        // This is the only way to ensure the splitter starts with the handle
        // halfway (only tested when starting in Split mode - might not work if
        // starting in Edit)
        Time::onNextTick(this, [this] {
            auto w = splitter_->width() / 2;
            splitter_->setSizes({ w, w });
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
                if (preview_ && preview_->isVisible()) reparseTimer_->start();
            });

        connect(modeToggle_, &QToolButton::clicked, this, [this] {
            cycleMode();
        });

        reparse_();

        return container_;
    }

    // Subclasses implement this to convert plain text to HTML for the preview
    virtual QString renderToHtml(const QString& plainText) const = 0;

    QWebEngineView* preview() const noexcept { return preview_; }

private:
    Mode mode_ = Split;
    QWidget* container_ = new QWidget(this);
    QWidget* modeBar_ = new QWidget(this);
    QToolButton* modeToggle_ = new QToolButton(this);
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    QWebEngineView* preview_ = new QWebEngineView(this);

    Time::Debouncer* reparseTimer_ =
        Time::newDebouncer(this, &AbstractMarkupFileView::reparse_, 250);

    constexpr static int MIN_WIDGET_SIZE_ = 50;
    bool webViewFirstLoad_ = true;

    static QString appFontFaceKit_();

    void reparse_()
    {
        auto editor = this->editor();
        if (!preview_ || !editor) return;

        auto html = renderToHtml(editor->document()->toPlainText());

        html.replace(
            QStringLiteral("</head>"),
            QStringLiteral("<style>%1</style></head>").arg(appFontFaceKit_()));

        if (webViewFirstLoad_) {
            webViewFirstLoad_ = false;
            preview_->setHtml(
                html,
                QUrl("qrc:/")); /// TODO MU: I am vaguely concerned about this

            return;
        }

        // Extract just the body content and swap it via DOM manipulation
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
