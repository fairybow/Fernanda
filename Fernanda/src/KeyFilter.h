/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QChar>
#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QTextCursor>
#include <QTextDocument>

#include "Debug.h"

namespace Fernanda {

// Event filter providing typing enhancements for text editors: auto-close
// (insert matching punctuation pairs), delete-pair (backspace removes both),
// skip-closer (typing a closer skips over it), and barging (double-space jumps
// past closing punctuation)
//
// TODO: Wrapping selection in open/close punctuation
class KeyFilter : public QObject
{
    Q_OBJECT

public:
    explicit KeyFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~KeyFilter() override { TRACER; }

    void setTextEdit(QPlainTextEdit* textEdit)
    {
        if (textEdit_) textEdit_->removeEventFilter(this);
        textEdit_ = textEdit;
        if (textEdit) textEdit->installEventFilter(this);
    }

    bool isActive() const noexcept { return active_; }
    void setActive(bool active) { active_ = active; }

    bool autoClosing() const noexcept { return autoClosing_; }
    void setAutoClosing(bool autoClosing) { autoClosing_ = autoClosing; }

    bool barging() const noexcept { return barging_; }
    void setBarging(bool barging) { barging_ = barging; }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (active_ && watched == textEdit_ && event->type() == QEvent::KeyPress
            && handleKeyPress_(static_cast<QKeyEvent*>(event)))
            return true;

        return QObject::eventFilter(watched, event);
    }

private:
    QPointer<QPlainTextEdit> textEdit_ = nullptr;

    // TODO: Settings!
    bool active_ = true;
    bool autoClosing_ = true;
    bool barging_ = true;

    bool handleKeyPress_(QKeyEvent* event)
    {
        if (!textEdit_) return false;
        if (!event) return false;

        auto document = textEdit_->document();
        auto cursor = textEdit_->textCursor();

        if (autoClosing_ && event->key() == Qt::Key_Backspace) {
            if (isBetweenPair_(document, cursor)) {
                deletePair_(cursor);
                textEdit_->setTextCursor(cursor);
                return true;
            }
        }

        if (barging_ && event->key() == Qt::Key_Space) {
            if (canBarge_(document, cursor)) {
                barge_(cursor);
                textEdit_->setTextCursor(cursor);
                return true;
            }
        }

        auto text = event->text();
        if (text.isEmpty()) return false;

        auto ch = text.at(0);

        // Skip-closer: typing a closer when same char is ahead
        if (autoClosing_ && canSkipCloser_(ch, document, cursor)) {
            cursor.movePosition(QTextCursor::NextCharacter);
            textEdit_->setTextCursor(cursor);
            return true;
        }

        if (autoClosing_ && isOpenPunct_(ch)) {
            // Single quote needs context check for contractions
            if (ch == '\'' && isMidWord_(document, cursor)) return false;
            autoClose_(ch, cursor);
            textEdit_->setTextCursor(cursor);

            return true;
        }

        return false;
    }

    void autoClose_(QChar ch, QTextCursor& cursor)
    {
        cursor.beginEditBlock();
        cursor.insertText(QString(ch) + ch.mirroredChar());
        cursor.movePosition(QTextCursor::PreviousCharacter);
        cursor.endEditBlock();
    }

    void deletePair_(QTextCursor& cursor)
    {
        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.deleteChar();
        cursor.endEditBlock();
    }

    void barge_(QTextCursor& cursor)
    {
        cursor.beginEditBlock();
        cursor.deletePreviousChar(); // remove the space
        cursor.movePosition(QTextCursor::NextCharacter); // skip past closer
        cursor.insertText(QStringLiteral(" ")); // add space after
        cursor.endEditBlock();
    }

    bool isOpenPunct_(QChar c) const noexcept
    {
        return c == '"' || c == '\'' || c.category() == QChar::Punctuation_Open
               || c.category() == QChar::Punctuation_InitialQuote;
    }

    bool isClosePunct_(QChar c) const noexcept
    {
        return c == '"' || c == '\'' || c.category() == QChar::Punctuation_Close
               || c.category() == QChar::Punctuation_FinalQuote;
    }

    bool canSkipCloser_(
        QChar ch,
        QTextDocument* document,
        const QTextCursor& cursor) const
    {
        if (!isClosePunct_(ch)) return false;
        if (!document) return false;

        auto pos = cursor.position();
        if (pos >= document->characterCount()) return false;

        return document->characterAt(pos) == ch;
    }

    bool isMidWord_(QTextDocument* document, const QTextCursor& cursor) const
    {
        if (!document) return false;

        auto pos = cursor.position();
        if (pos == 0) return false;

        return document->characterAt(pos - 1).isLetterOrNumber();
    }

    bool
    isBetweenPair_(QTextDocument* document, const QTextCursor& cursor) const
    {
        if (!document) return false;

        auto pos = cursor.position();
        if (pos == 0) return false;

        if (pos >= document->characterCount()) return false;

        auto char_before = document->characterAt(pos - 1);
        auto char_after = document->characterAt(pos);

        return isOpenPunct_(char_before)
               && char_after == char_before.mirroredChar();
    }

    bool canBarge_(QTextDocument* document, const QTextCursor& cursor) const
    {
        if (!document) return false;

        auto pos = cursor.position();
        if (pos < 1) return false;
        if (pos >= document->characterCount()) return false;

        auto char_before = document->characterAt(pos - 1);
        auto char_after = document->characterAt(pos);

        if (!(char_before == ' ' && isClosePunct_(char_after))) return false;

        // Don't barge past single quote if it looks like a contraction
        if (char_after == '\'' && pos >= 2
            && document->characterAt(pos - 2).isLetterOrNumber())
            return false;

        return true;
    }
};

} // namespace Fernanda
