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

#include "ui/PlainTextEdit.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QTextBlock>
#include <QWidget>

#include "core/Application.h"

namespace Hearth {

PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setup_();
}

void PlainTextEdit::setLineNumbers(bool lineNumbers)
{
    lineNumbers_ = lineNumbers;
    lineNumberArea_->setVisible(lineNumbers);
    updateViewportMargins_();
}

void PlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea_);

    auto bg_width = lineNumberArea_->width() - Internal::LNA_LINE_RIGHT_PADDING_
                    - Internal::LNA_LINE_THICKNESS_;

    painter.fillRect(
        event->rect().left(),
        event->rect().top(),
        bg_width - event->rect().left(),
        event->rect().height(),
        lineNumbersBackgroundColor_);

    painter.setFont(font());

    auto block = firstVisibleBlock();
    auto block_number = block.blockNumber();
    auto top =
        qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    auto bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            auto number = QString::number(block_number + 1);
            painter.setPen(lineNumbersColor_);
            painter.drawText(
                0,
                top,
                lineNumberArea_->width() - Internal::LNA_RIGHT_RESERVATION_,
                fontMetrics().height(),
                Qt::AlignRight,
                number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());

        ++block_number;
    }

    // LNA separator
    painter.fillRect(
        lineNumberArea_->width() - Internal::LNA_LINE_RIGHT_PADDING_
            - Internal::LNA_LINE_THICKNESS_,
        event->rect().top(),
        Internal::LNA_LINE_THICKNESS_,
        event->rect().height(),
        lineNumbersBorderColor_);
}

void PlainTextEdit::resizeEvent(QResizeEvent* event)
{
    updateViewportMargins_();
    QPlainTextEdit::resizeEvent(event);

    auto cr = contentsRect();
    lineNumberArea_->setGeometry(
        { cr.left(), cr.top(), lineNumberAreaWidth(), cr.height() });
}

void PlainTextEdit::setup_()
{
    lineNumberArea_ = new LineNumberArea(this);
    lineNumberArea_->setVisible(lineNumbers_);

    setViewportMargins(0, 0, 0, 0);

    connect(
        this,
        &PlainTextEdit::cursorPositionChanged,
        this,
        &PlainTextEdit::resetCursorBlink_);

    connect(this, &PlainTextEdit::blockCountChanged, this, [this] {
        updateViewportMargins_();
    });

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

    updateViewportMargins_();
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

void PlainTextEdit::updateLineNumberArea_(const QRect& rect, int deltaY)
{
    if (deltaY) {
        lineNumberArea_->scroll(0, deltaY);
    } else {
        lineNumberArea_
            ->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) updateViewportMargins_();
}

} // namespace Hearth
