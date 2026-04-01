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
#include <QStackedWidget>
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

class AbstractMarkupFileView : public TextFileView
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
        if (!splitter_ || !preview_) return;

        mode_ = mode;
        auto editor = this->editor();

        switch (mode) {

        case Edit: {
            editor->show();
            previewStack_->hide();
            splitter_->setFocusProxy(editor);

            break;
        }

        case Split: {
            editor->show();
            previewStack_->show();

            if (needsInitialSplit_) {
                previewStack_->setMinimumWidth(splitter_->width() / 2);
                previewStack_->setMinimumWidth(MIN_WIDGET_WIDTH_);

                needsInitialSplit_ = false;
            }

            splitter_->setFocusProxy(editor);

            break;
        }

        case Preview: {
            editor->hide();
            previewStack_->show();
            splitter_->setFocusProxy(previewStack_);

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
        auto editor_widget = TextFileView::setupWidget();
        editor_widget->setMinimumWidth(MIN_WIDGET_WIDTH_);

        modeBar_->setFixedHeight(24);

        toggleButton_->setText(
            "Toggle Mode"); /// TODO MU: Reactive icon or switch
        toggleButton_->setFixedHeight(20);

        previewLoadingMask_->setStyleSheet(
            QStringLiteral("background-color: black;"));
        preview_->setPage(new MarkupPreviewPage(preview_));

        previewStack_->addWidget(previewLoadingMask_); // 0
        previewStack_->addWidget(preview_); // 1
        previewStack_->setCurrentWidget(previewLoadingMask_);

        splitter_->addWidget(editor_widget);
        splitter_->addWidget(previewStack_);

        // Needed to allow the min size setting in Split case above to not push
        // splitter handle too far
        splitter_->setStretchFactor(0, 1);
        splitter_->setStretchFactor(1, 0);
        splitter_->setChildrenCollapsible(false);

        auto container_layout = new QVBoxLayout(container_);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);

        auto mode_bar_layout = new QHBoxLayout(modeBar_);
        mode_bar_layout->setContentsMargins(0, 2, 2, 2);
        mode_bar_layout->setSpacing(0);

        mode_bar_layout->addStretch();
        mode_bar_layout->addWidget(toggleButton_, 0);

        container_layout->addWidget(modeBar_, 0);
        container_layout->addWidget(splitter_, 1);

        connect(
            editor()->document(),
            &QTextDocument::contentsChanged,
            this,
            [this] {
                if (preview_ && preview_->isVisible()) reparseTimer_->start();
            });

        connect(toggleButton_, &QToolButton::clicked, this, [this] {
            cycleMode();
        });

        connect(preview_, &QWebEngineView::loadStarted, this, [this] {
            previewStack_->setCurrentWidget(previewLoadingMask_);
        });

        connect(preview_, &QWebEngineView::loadFinished, this, [this](bool) {
            //previewStack_->setCurrentWidget(preview_);
            Time::delay(50, this, [this] {
                previewStack_->setCurrentWidget(preview_);
            });
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
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    QStackedWidget* previewStack_ = new QStackedWidget(this);
    QWidget* previewLoadingMask_ =
        new QWidget(this); /// Make white and maybe add spinner
    QWebEngineView* preview_ = new QWebEngineView(this);

    Time::Debouncer* reparseTimer_ =
        Time::newDebouncer(this, &AbstractMarkupFileView::reparse_, 250);

    constexpr static auto MIN_WIDGET_WIDTH_ = 50;
    bool needsInitialSplit_ = true;
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
};

} // namespace Fernanda
