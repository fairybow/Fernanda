/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QChar>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPoint>
#include <QString>
#include <QTextCursor>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit(parent)
    {
        setup_();
    }

    virtual ~PlainTextEdit() override { TRACER; }

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        // Allow double-clicking white space runs (except single spaces and new
        // lines)

        auto cursor = cursorForPosition(event->pos());
        auto position = static_cast<qsizetype>(cursor.position());
        auto text = document()->toPlainText();

        if (position < text.size() && text[position].isSpace()
            && text[position] != '\n') {
            // Expand left
            auto start = position;
            while (start > 0 && text[start - 1].isSpace()
                   && text[start - 1] != '\n')
                --start;

            // Expand right
            auto end = position;
            while (end < text.size() && text[end].isSpace()
                   && text[end] != '\n')
                ++end;

            if (!(end - start == 1 && text[start] == ' ')) {
                cursor.setPosition(start);
                cursor.setPosition(end, QTextCursor::KeepAnchor);
                setTextCursor(cursor);

                return;
            }
        }

        QPlainTextEdit::mouseDoubleClickEvent(event);
    }

private:
    void setup_()
    {
        setViewportMargins(5, 0, 5, 0);

        connect(
            this,
            &PlainTextEdit::cursorPositionChanged,
            this,
            &PlainTextEdit::onCursorPositionChanged_);
    }

private slots:
    void onCursorPositionChanged_();
};

} // namespace Fernanda
