/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "PlainTextEdit.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QTextBlock>
#include <QWidget>

#include "Application.h"

namespace Fernanda {

PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setup_();
}

/// TODO LNA
void PlainTextEdit::setLineNumbers(bool lineNumbers)
{
    lineNumbers_ = lineNumbers;
    lineNumberArea_->setVisible(lineNumbers);
    updateLineNumberAreaWidth_(0);
}

/// TODO LNA
void PlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea_);
    /// TODO LNA: Style context color here
    painter.fillRect(event->rect(), Qt::lightGray);
    painter.setFont(font());

    auto block = firstVisibleBlock();
    auto block_number = block.blockNumber();
    auto top =
        qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    auto bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            auto number = QString::number(block_number + 1);
            /// TODO LNA: Style context color here
            /// TODO LNA: Set font, too (same as editor)
            painter.setPen(Qt::black);
            painter.drawText(
                0,
                top,
                lineNumberArea_->width(),
                fontMetrics().height(),
                Qt::AlignRight,
                number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());

        ++block_number;
    }
}

/// TODO LNA
void PlainTextEdit::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    auto cr = contentsRect();
    lineNumberArea_->setGeometry(
        { cr.left(), cr.top(), lineNumberAreaWidth(), cr.height() });
}

void PlainTextEdit::setup_()
{
    setViewportMargins(0, 0, 0, 0);

    connect(
        this,
        &PlainTextEdit::cursorPositionChanged,
        this,
        &PlainTextEdit::resetCursorBlink_);

    /// TODO LNA:

    lineNumberArea_ = new LineNumberArea(this);
    lineNumberArea_->setVisible(lineNumbers_);

    connect(
        this,
        &PlainTextEdit::blockCountChanged,
        this,
        &PlainTextEdit::updateLineNumberAreaWidth_);

    connect(
        this,
        &PlainTextEdit::updateRequest,
        this,
        &PlainTextEdit::updateLineNumberArea_);

    connect(
        this,
        &PlainTextEdit::cursorPositionChanged,
        this,
        &PlainTextEdit::highlightCurrentLine_);

    updateLineNumberAreaWidth_(0);
    highlightCurrentLine_();
}

void PlainTextEdit::resetCursorBlink_()
{
    // Ensuring cursor is visible by just calling `setTextCursor(textCursor())`
    // is not ideal, since though it does work, it doesn't change the
    // application's universal cursor blink timer, meaning the cursor is visible
    // for an inconsistent amount of time after each move. It looks and feels
    // bad! This does not do that:

    static auto original_flash_time = -1;

    if (original_flash_time < 0) {
        original_flash_time = Application::cursorFlashTime();
    }

    Application::setCursorFlashTime(0);
    Application::setCursorFlashTime(original_flash_time);
}

/// TODO LNA
void PlainTextEdit::updateLineNumberArea_(const QRect& rect, int deltaY)
{
    if (deltaY)
        lineNumberArea_->scroll(0, deltaY);
    else
        lineNumberArea_
            ->update(0, rect.y(), lineNumberArea_->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth_(0);
}

} // namespace Fernanda
