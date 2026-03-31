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

#include <QScrollBar>
#include <QSplitter>
#include <QString>
#include <QTextDocument>
#include <QWebEngineView>
#include <QWidget>

#include "core/Time.h"
#include "models/TextFileModel.h"
#include "views/TextFileView.h"

namespace Fernanda {

// TODO: Start edit only and load preview in the background? It's only slow in
// debug, though
// TODO: Toggle/cycle button
// TODO: ^ Would prefer more distinctive toggle modes (not moving a splitter);
// possible a stacked widget that displays the right modes between edit, split,
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

        splitter_ = new QSplitter(Qt::Horizontal, this);
        splitter_->addWidget(editor_widget);
        splitter_->addWidget(preview_);
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

        if (preview_->url().isEmpty()) {
            // First load (no scroll to preserve)
            preview_->setHtml(html);
            return;
        }

        // Subsequent loads (preserve scroll)
        preview_->page()->runJavaScript(
            "window.scrollY",
            [this, html](const QVariant& scrollPos) {
                auto y = scrollPos.toInt();
                preview_->setHtml(html);
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

} // namespace Fernanda
