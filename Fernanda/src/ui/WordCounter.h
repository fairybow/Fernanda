/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <QChar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/TextCounts.h"
#include "core/VaultConfig.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The status bar's word counter: the words, characters, and lines of the
// window's active text view, as "1,234 words · 5,678 characters". With a
// selection each count reads "12 of 1,234 words" — the selection's count
// against the document's — as Word and Google Docs show it. What counts as a
// word or a character is TextCounts' business; a line is a hard line (a
// QTextBlock), as it is for CursorPosition.
//
// Shown only while it has an editor to read, exactly as CursorPosition: the
// window hands it one for a text view and nullptr otherwise, and with nullptr
// it hides. It also hides when switched off in the vault's settings, or when
// every count is — and counts nothing it isn't showing.
//
// The document's counts are kept and recounted in full a moment after the text
// stops changing (RECOUNT_DEBOUNCE_MS_), by walking its blocks — no copy of the
// whole text is made. That's a few milliseconds for a novel-length file and it
// only ever runs for the active view, so there is no manual-refresh mode for
// big documents. If a huge file ever makes the pause after typing felt, the
// next step is keeping each block's counts and recounting only edited blocks.
// The selection's counts aren't kept: they're recounted from the selected text
// on each selection change, which costs in proportion to the selection
class WordCounter : public QLabel
{
    Q_OBJECT

public:
    // Handed to the window's status bar, which reparents it
    explicit WordCounter(QWidget* parentWindow)
        : QLabel(parentWindow)
    {
        setup_();
    }

    ~WordCounter() override { TRACER; }

    // The editor to count, or nullptr for none. Borrowed: the editor belongs
    // to its view, which can be destroyed at any time (a closed tab, a file
    // deleted on disk), so its destroyed is watched and clears it here
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
            // Typing restarts the wait, so a burst of edits is one recount.
            // Also fires for text arriving from another view of the same file,
            // and for a reload from disk
            connect(editor_, &QPlainTextEdit::textChanged, this, [this] {
                if (shown_.enabled) {
                    recountTimer_->start();
                }
            });

            connect(
                editor_,
                &QPlainTextEdit::selectionChanged,
                this,
                &WordCounter::updateDisplay_);

            connect(editor_, &QObject::destroyed, this, [this] {
                editor_ = nullptr;
                recount_();
            });
        }

        // A newly active view shows its counts at once, not after the wait
        recount_();
    }

    // What to show, from the hosting vault's config (BaseWindow::applyConfig).
    // Called on every config change, most of which are other settings (a
    // dragged font-size slider, many times a second), so one that leaves
    // these alone must cost nothing — a real change recounts, since a count
    // that was off was never taken
    void applyConfig(const VaultConfig& config)
    {
        Shown_ shown{ .enabled = config.wordCounterEnabled(),
                      .words = config.wordCounterWords(),
                      .characters = config.wordCounterCharacters(),
                      .lines = config.wordCounterLines(),
                      .selection = config.wordCounterSelection() };

        if (shown == shown_) {
            return;
        }

        shown_ = shown;
        recount_();
    }

private:
    struct Counts_
    {
        int words = 0;
        int characters = 0;
        int lines = 0;
    };

    // The item's own switch, then each thing it can show. selection is
    // whether a selection's counts are shown against the document's
    struct Shown_
    {
        bool enabled = VaultConfig::DEFAULT_WORD_COUNTER_ENABLED;
        bool words = VaultConfig::DEFAULT_WORD_COUNTER_WORDS;
        bool characters = VaultConfig::DEFAULT_WORD_COUNTER_CHARACTERS;
        bool lines = VaultConfig::DEFAULT_WORD_COUNTER_LINES;
        bool selection = VaultConfig::DEFAULT_WORD_COUNTER_SELECTION;

        bool operator==(const Shown_&) const = default;
    };

    static constexpr int RECOUNT_DEBOUNCE_MS_ = 150;

    QPlainTextEdit* editor_ = nullptr;

    // The document's counts as of the last recount_. A selection change
    // reads these; it never recounts the document
    Counts_ totals_{};

    Coco::Time::Debouncer* recountTimer_ = Coco::Time::newDebouncer(
        this,
        &WordCounter::recount_,
        RECOUNT_DEBOUNCE_MS_);

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

    // Count the whole document and show the result. Runs when the editor or
    // the settings change and when the debounce fires
    void recount_()
    {
        recountTimer_->stop();
        totals_ = (editor_ && shown_.enabled) ? countDocument_() : Counts_{};
        updateDisplay_();
    }

    // Words and characters are only counted if they're shown — each is a
    // pass over the text. Lines are the document's own block count
    [[nodiscard]] Counts_ countDocument_() const
    {
        Counts_ counts{};
        auto* document = editor_->document();

        if (shown_.words || shown_.characters) {
            for (auto block = document->begin(); block.isValid();
                 block = block.next()) {
                addLine_(counts, block.text());
            }
        }

        counts.lines = document->blockCount();
        return counts;
    }

    // QTextCursor::selectedText marks each line break with U+2029, never a
    // newline, so the selection is split there and counted a line at a time,
    // as the document is. Its line count is the lines it touches
    [[nodiscard]] Counts_ countSelection_(const QTextCursor& cursor) const
    {
        Counts_ counts{};
        auto selected = cursor.selectedText();
        QStringView rest(selected);

        while (true) {
            auto end = rest.indexOf(QChar::ParagraphSeparator);
            ++counts.lines;

            if (end < 0) {
                addLine_(counts, rest);
                break;
            }

            addLine_(counts, rest.first(end));
            rest = rest.sliced(end + 1);
        }

        return counts;
    }

    void addLine_(Counts_& counts, QStringView line) const
    {
        if (shown_.words) {
            counts.words += TextCounts::wordCount(line);
        }

        if (shown_.characters) {
            counts.characters += TextCounts::characterCount(line);
        }
    }

    // Rebuild the text from totals_ and the current selection. With every
    // count switched off there is nothing to show, and it hides
    void updateDisplay_()
    {
        if (!editor_ || !shown_.enabled) {
            hide();
            clear();
            return;
        }

        auto cursor = editor_->textCursor();
        auto selecting = shown_.selection && cursor.hasSelection();
        auto selected = selecting ? countSelection_(cursor) : Counts_{};

        QStringList parts{};

        if (shown_.words) {
            //: Status bar word count. The first form is the whole document;
            //: the second is shown while text is selected, where %L1 is the
            //: selection's word count and %Ln the document's.
            parts
                << (selecting ? tr("%L1 of %Ln word(s)", nullptr, totals_.words)
                                    .arg(selected.words)
                              : tr("%Ln word(s)", nullptr, totals_.words));
        }

        if (shown_.characters) {
            //: Status bar character count; forms as for the word count.
            parts
                << (selecting
                        ? tr("%L1 of %Ln character(s)",
                             nullptr,
                             totals_.characters)
                              .arg(selected.characters)
                        : tr("%Ln character(s)", nullptr, totals_.characters));
        }

        if (shown_.lines) {
            //: Status bar line count; forms as for the word count.
            parts
                << (selecting ? tr("%L1 of %Ln line(s)", nullptr, totals_.lines)
                                    .arg(selected.lines)
                              : tr("%Ln line(s)", nullptr, totals_.lines));
        }

        if (parts.isEmpty()) {
            hide();
            clear();
            return;
        }

        setText(parts.join(u" \u00B7 "_s));
        show();
    }
};

} // namespace Fernanda::Ui
