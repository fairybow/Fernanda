/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QPlainTextEdit>
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
