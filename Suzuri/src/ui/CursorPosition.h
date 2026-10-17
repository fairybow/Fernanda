/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QLabel>
#include <QPlainTextEdit>
#include <QString>
#include <QStringView>
#include <QTextBlock>
#include <QTextCursor>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/TextCounts.h"
#include "core/VaultConfig.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"

namespace Suzuri::Ui {

// The status bar's cursor position: the line and column of the caret in the
// window's active text view, as "Ln 12, Col 5". Shown only while it has an
// editor to read — the window hands it one when the active page is a text
// view and nullptr otherwise (a PDF, an image, a New Tab page, an empty
// pane), and with nullptr it hides, taking no room in the bar. It also hides
// when switched off in the vault's settings, or when both its parts are.
//
// A line is a hard line (a QTextBlock), not a wrapped visual line, so a
// paragraph that wraps across the editor is still one line — what Notepad and
// VS Code report. A column is the caret's character in that line, counted as
// TextCounts counts characters: a tab or an emoji is one column. With a
// selection, the position is the end that moves.
//
// It reads the editor directly and keeps nothing: each caret move or text
// change recomputes two numbers from the caret's own line, which is cheap
// enough to do unconditionally
class CursorPosition : public QLabel
{
    Q_OBJECT

public:
    // Handed to the window's status bar, which reparents it
    explicit CursorPosition(QWidget* parentWindow)
        : QLabel(parentWindow)
    {
        setup_();
    }

    ~CursorPosition() override { TRACER; }

    // The editor to report on, or nullptr for none. Borrowed: the editor
    // belongs to its view, which can be destroyed at any time (a closed tab,
    // a file deleted on disk), so its destroyed is watched and clears it here
    void setEditor(QPlainTextEdit* editor)
    {
        if (editor == editor_) {
            return;
        }

        if (editor_) {
            editor_->disconnect(this);
        }

        editor_ = editor;

        if (editor_) {
            connect(
                editor_,
                &QPlainTextEdit::cursorPositionChanged,
                this,
                &CursorPosition::refresh_);

            // A caret move announces itself, but text arriving from another
            // view of the same file can push this caret to a new line or column
            // without one
            connect(
                editor_,
                &QPlainTextEdit::textChanged,
                this,
                &CursorPosition::refresh_);

            connect(editor_, &QObject::destroyed, this, [this] {
                editor_ = nullptr;
                refresh_();
            });
        }

        refresh_();
    }

    // What to show, from the hosting vault's config (BaseWindow::applyConfig).
    // Called on every config change, most of which are other settings, so
    // one that leaves these alone does nothing
    void applyConfig(const VaultConfig& config)
    {
        Shown_ shown{ .enabled = config.cursorPositionEnabled(),
                      .line = config.cursorPositionLine(),
                      .column = config.cursorPositionColumn() };

        if (shown == shown_) {
            return;
        }

        shown_ = shown;
        refresh_();
    }

private:
    // The item's own switch, then each part it can show
    struct Shown_
    {
        bool enabled = VaultConfig::DEFAULT_CURSOR_POSITION_ENABLED;
        bool line = VaultConfig::DEFAULT_CURSOR_POSITION_LINE;
        bool column = VaultConfig::DEFAULT_CURSOR_POSITION_COLUMN;

        bool operator==(const Shown_&) const = default;
    };

    QPlainTextEdit* editor_ = nullptr;

    // The config's defaults until applyConfig says otherwise
    Shown_ shown_{};

    void setup_()
    {
        setForegroundRole(STATUS_BAR_TEXT_ROLE);
        setContentsMargins(
            STATUS_BAR_ITEM_H_PADDING,
            0,
            STATUS_BAR_ITEM_H_PADDING,
            0);
        setTabularNumerals(this);

        // Hidden before the status bar takes it: the bar shows what it's
        // given unless it was explicitly hidden first
        hide();
    }

    // With no editor, the item off, or both parts off, there is nothing to
    // show, and it hides
    void refresh_()
    {
        if (!editor_ || !shown_.enabled || (!shown_.line && !shown_.column)) {
            hide();
            clear();
            return;
        }

        auto cursor = editor_->textCursor();
        auto line = cursor.blockNumber() + 1;

        // Only measured if it's shown: it reads the caret's whole line
        auto column = 0;
        if (shown_.column) {
            auto line_text = cursor.block().text();
            auto before_caret =
                QStringView(line_text).first(cursor.positionInBlock());
            column = TextCounts::characterCount(before_caret) + 1;
        }

        if (shown_.line && shown_.column) {
            //: The caret's line and column in the status bar: %L1 is the line
            //: number, %L2 the column number.
            setText(tr("Ln %L1, Col %L2").arg(line).arg(column));
        } else if (shown_.line) {
            //: The caret's line in the status bar: %L1 is the line number.
            setText(tr("Ln %L1").arg(line));
        } else {
            //: The caret's column in the status bar: %L1 is the column number.
            setText(tr("Col %L1").arg(column));
        }

        show();
    }
};

} // namespace Suzuri::Ui
