#pragma once

#include <QChar>
#include <QKeyEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QtTypes>

#include "Coco/Debug.h"

class KeyFilter : public QObject
{
    Q_OBJECT

public:
    explicit KeyFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~KeyFilter() override { COCO_TRACER; }

    bool handle(QPlainTextEdit* textEdit, QKeyEvent* event)
    {
        if (!textEdit || !event) return false;

        auto text = event->text();
        if (text.isEmpty()) return false;
        auto c = text.at(0);

        if (isOpenPunct_(c))
        {
            autoClose_(textEdit, c);
            event->ignore();
            return true;
        }

        return false;
    }

private:
    bool isOpenPunct_(QChar c) const noexcept
    {
        return c == '"'
            || c.category() == QChar::Punctuation_Open
            || c.category() == QChar::Punctuation_InitialQuote;
    }

    // For barging
    bool isClosePunct_(QChar c) const noexcept
    {
        return c == '"'
            || c.category() == QChar::Punctuation_Close
            || c.category() == QChar::Punctuation_FinalQuote;
    }

    void autoClose_(QPlainTextEdit* textEdit, QChar c)
    {
        // Adds the corresponding closing punctuation and positions the cursor
        // in between the two

        if (!textEdit || c.isNull()) return;
        auto cursor = textEdit->textCursor();

        cursor.beginEditBlock();
        cursor.insertText(c);
        cursor.endEditBlock();

        cursor.beginEditBlock();
        cursor.insertText(c.mirroredChar());
        cursor.movePosition(QTextCursor::PreviousCharacter);
        cursor.endEditBlock();

        textEdit->setTextCursor(cursor);
    }
};
