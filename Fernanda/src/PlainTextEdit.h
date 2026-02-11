/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QBrush>
#include <QChar>
#include <QColor>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QList>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QSize>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QVariant>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class LineNumberArea; /// TODO LNA

// TODO: Grabbable highlights (Check in old "hold" folder but revise)
// TODO: StyleContext for LNA bg, LNA font color, line highlight color
// TODO: Setting for changing which line numbers display (every 5, every 4, etc, maybe)
class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr);
    virtual ~PlainTextEdit() override { TRACER; }

    // TODO: Rename this property? Not very clear
    bool doubleClickWhitespace() const { return doubleClickWhitespace_; }

    void setDoubleClickWhitespace(bool doubleClickWhitespace)
    {
        doubleClickWhitespace_ = doubleClickWhitespace;
    }

    bool lineNumbers() const { return lineNumbers_; }
    void setLineNumbers(bool lineNumbers);
    bool lineHighlight() const { return lineHighlight_; }

    void setLineHighlight(bool lineHighlight)
    {
        lineHighlight_ = lineHighlight;
        highlightCurrentLine_();
    }

    /// TODO LNA
    void lineNumberAreaPaintEvent(QPaintEvent* event);

    /// TODO LNA
    int lineNumberAreaWidth()
    {
        if (!lineNumbers_) return 0;

        auto digits = 1;
        auto max = qMax(1, blockCount());

        while (max >= 10) {
            max /= 10;
            ++digits;
        }

        auto space =
            3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

        return space;
    }

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (doubleClickWhitespace_) {
            // Allow double-clicking whitespace runs (except single spaces and
            // new lines)
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
        }

        QPlainTextEdit::mouseDoubleClickEvent(event);
    }

    /// TODO LNA
    void resizeEvent(QResizeEvent* event) override;

private:
    /// TODO LNA
    QWidget* lineNumberArea_ = nullptr;

    bool doubleClickWhitespace_ = true;
    bool lineNumbers_ = true;
    bool lineHighlight_ = true;

    void setup_();

private slots:
    void resetCursorBlink_();

    /// TODO LNA
    void updateLineNumberAreaWidth_(int newBlockCount)
    {
        (void)newBlockCount;
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    /// TODO LNA
    void highlightCurrentLine_()
    {
        QList<QTextEdit::ExtraSelection> extra_selections{};

        if (lineHighlight_  && !isReadOnly()) {
            QTextEdit::ExtraSelection selection{};
            /// TODO LNA: style context value
            auto line_color = QColor(Qt::yellow).lighter(160);

            selection.format.setBackground(line_color);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();

            extra_selections << selection;
        }

        setExtraSelections(extra_selections);
    }

    /// TODO LNA
    void updateLineNumberArea_(const QRect& rect, int deltaY);
};

/// TODO LNA
// See:
// https://doc.qt.io/archives/qt-5.15/qtwidgets-widgets-codeeditor-example.html
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PlainTextEdit* plainTextEdit)
        : QWidget(plainTextEdit)
        , plainTextEdit_(plainTextEdit)
    {
    }

    virtual ~LineNumberArea() override { TRACER; }

    QSize sizeHint() const override
    {
        return { plainTextEdit_->lineNumberAreaWidth(), 0 };
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        plainTextEdit_->lineNumberAreaPaintEvent(event);
    }

private:
    PlainTextEdit* plainTextEdit_;
};

} // namespace Fernanda
