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

// TODO: Start edit only and load preview in the background? It's only slow in
// debug, though
// TODO: Toggle/cycle button
// TODO: ^ Would prefer more distinctive toggle modes (not moving a splitter);
// possibly a stacked widget that displays the right modes between edit, split,
// and preview. Though, this would mean moving the two widgets into a splitter
// for split or having duplicates?
class AbstractMarkupFileView : public TextFileView
{
    Q_OBJECT

public:
    enum Mode
    {
        Split,
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
        auto editor_widget = TextFileView::setupWidget();

        preview_ = new QWebEngineView(this);
        preview_->setPage(new MarkupPreviewPage(preview_));

        splitter_ = new QSplitter(Qt::Horizontal, this);
        splitter_->addWidget(editor_widget);
        splitter_->addWidget(preview_);
        splitter_->setChildrenCollapsible(false);
        splitter_->setFocusProxy(editor_widget);

        connect(
            editor()->document(),
            &QTextDocument::contentsChanged,
            this,
            [this] {
                if (preview_ && preview_->isVisible()) reparseTimer_->start();
            });

        reparse_();

        return splitter_;
    }

    // Subclasses implement this to convert plain text to HTML for the preview
    virtual QString renderToHtml(const QString& plainText) const = 0;

    QWebEngineView* preview() const noexcept { return preview_; }

private:
    QSplitter* splitter_ = nullptr;
    QWebEngineView* preview_ = nullptr;
    Mode mode_ = Split;
    Time::Debouncer* reparseTimer_ =
        Time::newDebouncer(this, &AbstractMarkupFileView::reparse_, 250);

    static QString appFontFaceKit_();

    // void reparse_()
    //{
    //     auto editor = this->editor();
    //     if (!preview_ || !editor) return;

    //    auto scroll = preview_->verticalScrollBar()->value();
    //    auto html = renderToHtml(editor->document()->toPlainText());
    //    preview_->setHtml(html);
    //    preview_->verticalScrollBar()->setValue(scroll);
    //}

    // TODO: This is buggy and jumpy af
    void reparse_()
    {
        auto editor = this->editor();
        if (!preview_ || !editor) return;

        auto html = renderToHtml(editor->document()->toPlainText());

        // Inject bundled font-face rules for the web engine
        html.replace(
            QStringLiteral("</head>"),
            QStringLiteral("<style>%1</style></head>").arg(appFontFaceKit_()));

        if (preview_->url().isEmpty()) {
            // First load (no scroll to preserve)
            preview_->setHtml(
                html,
                QUrl("qrc:/")); /// TODO MU: I am vaguely concerned about this
            return;
        }

        // Subsequent loads (preserve scroll)
        preview_->page()->runJavaScript(
            "window.scrollY",
            [this, html](const QVariant& scrollPos) {
                auto y = scrollPos.toInt();
                preview_->setHtml(html, QUrl("qrc:/")); /// TODO MU: See above
                connect(
                    preview_,
                    &QWebEngineView::loadFinished,
                    this,
                    [this, y](bool) {
                        preview_->page()->runJavaScript(
                            QString("window.scrollTo(0, %1)").arg(y));
                    },
                    Qt::SingleShotConnection);
            });
    }
};

/*class AbstractMarkupFileView : public TextFileView
{
    Q_OBJECT

public:
    enum Mode
    {
        Edit = 0,
        Split,
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
        if (!stack_ || !preview_) return;

        mode_ = mode;
        auto editor = this->editor();

        switch (mode) {

        case Edit: {
            editPage_->layout()->addWidget(editor);
            stack_->setCurrentWidget(editPage_);
            stack_->setFocusProxy(editor);

            break;
        }

        case Split: {
            splitter_->addWidget(editor);
            splitter_->addWidget(preview_);
            stack_->setCurrentWidget(splitPage_);
            stack_->setFocusProxy(editor);

            break;
        }

        case Preview: {
            previewPage_->layout()->addWidget(preview_);
            stack_->setCurrentWidget(previewPage_);
            stack_->setFocusProxy(preview_);

            break;
        }
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

        /// TODO MU: Reactive icon or switch
        toggleButton_->setText("Toggle Mode");
        toggleButton_->setFixedHeight(20);

        // --- Edit page ---
        auto editor_widget = TextFileView::setupWidget();
        auto edit_layout = new QVBoxLayout(editPage_);
        edit_layout->setContentsMargins(0, 0, 0, 0);
        edit_layout->setSpacing(0);
        edit_layout->addWidget(editor_widget);

        // --- Split page ---
        auto split_layout = new QVBoxLayout(splitPage_);
        split_layout->setContentsMargins(0, 0, 0, 0);
        split_layout->setSpacing(0);
        splitter_->setChildrenCollapsible(false);
        split_layout->addWidget(splitter_);

        // --- Preview page ---
        auto preview_layout = new QVBoxLayout(previewPage_);
        preview_layout->setContentsMargins(0, 0, 0, 0);
        preview_layout->setSpacing(0);
        preview_->setPage(new MarkupPreviewPage(preview_));

        // --- Stack ---
        stack_->addWidget(editPage_); // 0
        stack_->addWidget(splitPage_); // 1
        stack_->addWidget(previewPage_); // 2

        // --- Container ---
        auto container_layout = new QVBoxLayout(container_);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);

        auto mode_bar_layout = new QHBoxLayout(modeBar_);
        mode_bar_layout->setContentsMargins(0, 2, 2, 2);
        mode_bar_layout->setSpacing(0);
        mode_bar_layout->addStretch();
        mode_bar_layout->addWidget(toggleButton_, 0);

        container_layout->addWidget(modeBar_, 0);
        container_layout->addWidget(stack_, 1);

        connect(
            editor()->document(),
            &QTextDocument::contentsChanged,
            this,
            [this] {
                if (mode_ != Edit) reparseTimer_->start();
            });

        connect(toggleButton_, &QToolButton::clicked, this, [this] {
            cycleMode();
        });

        setMode(Edit);

        return container_;
    }

    // Subclasses implement this to convert plain text to HTML for the preview
    virtual QString renderToHtml(const QString& plainText) const = 0;

    QWebEngineView* preview() const noexcept { return preview_; }

private:
    Mode mode_ = Edit;
    QWidget* container_ = new QWidget(this);
    QWidget* modeBar_ = new QWidget(this);
    QToolButton* toggleButton_ = new QToolButton(this);

    QStackedWidget* stack_ = new QStackedWidget(this);
    QWidget* editPage_ = new QWidget(this);
    QWidget* splitPage_ = new QWidget(this);
    QWidget* previewPage_ = new QWidget(this);

    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    // QStackedWidget* previewStack_ = new QStackedWidget(this);
    // QWidget* previewLoadingMask_ =
    // new QWidget(this); /// Make white and maybe add spinner
    QWebEngineView* preview_ = new QWebEngineView(this);

    Time::Debouncer* reparseTimer_ =
        Time::newDebouncer(this, &AbstractMarkupFileView::reparse_, 250);

    // constexpr static auto MIN_WIDGET_WIDTH_ = 50;
    // bool needsInitialSplit_ = true;
    bool initialLoadDone_ = false;

    static QString appFontFaceKit_();

    void reparse_()
    {
        auto editor = this->editor();
        if (!preview_ || !editor) return;

        auto html = renderToHtml(editor->document()->toPlainText());

        html.replace(
            QStringLiteral("</head>"),
            QStringLiteral("<style>%1</style></head>").arg(appFontFaceKit_()));

        if (!initialLoadDone_) {
            initialLoadDone_ = true;
            preview_->setHtml(html, QUrl("qrc:/"));
            return;
        }

        // Extract just the body content and swap it via DOM manipulation
        auto body_start = html.indexOf(QStringLiteral("<body>"));
        auto body_end = html.indexOf(QStringLiteral("</body>"));

        if (body_start < 0 || body_end < 0) {
            preview_->setHtml(html, QUrl("qrc:/"));
            return;
        }

        auto body_content = html.mid(body_start + 6, body_end - body_start - 6);

        // Escape for JS string
        body_content.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        body_content.replace(QStringLiteral("`"), QStringLiteral("\\`"));

        auto js = QStringLiteral(
                      "var scrollY = window.scrollY;"
                      "document.body.innerHTML = `%1`;"
                      "window.scrollTo(0, scrollY);")
                      .arg(body_content);

        preview_->page()->runJavaScript(js);
    }
};*/

} // namespace Fernanda
