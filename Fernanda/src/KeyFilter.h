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

    bool autoClose() const noexcept { return autoClose_; }
    void setAutoClose(bool autoClose) { autoClose_ = autoClose; }

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
    bool autoClose_ = true;

    bool handleKeyPress_(QKeyEvent* event)
    {
        if (!textEdit_) return false;
        if (!event) return false;

        auto document = textEdit_->document();
        auto cursor = textEdit_->textCursor();

        if (autoClose_ && event->key() == Qt::Key_Backspace) {
            if (isBetweenPair_(document, cursor)) {
                deletePair_(cursor);
                textEdit_->setTextCursor(cursor);
                return true;
            }
        }

        auto text = event->text();
        if (text.isEmpty()) return false;

        auto ch = text.at(0);

        if (autoClose_ && isOpenPunct_(ch)) {
            // Single quote needs context check for contractions
            if (ch == '\'' && isMidWord_(document, cursor)) return false;

            cursor.beginEditBlock();
            cursor.insertText(QString(ch) + ch.mirroredChar());
            cursor.movePosition(QTextCursor::PreviousCharacter);
            cursor.endEditBlock();

            textEdit_->setTextCursor(cursor);
            return true;
        }

        return false;
    }

    bool isOpenPunct_(QChar c) const noexcept
    {
        return c == '"' || c == '\'' || c.category() == QChar::Punctuation_Open
               || c.category() == QChar::Punctuation_InitialQuote;
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

    void deletePair_(QTextCursor& cursor)
    {
        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.deleteChar();
        cursor.endEditBlock();
    }
};

} // namespace Fernanda
