/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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
#include <QKeyEvent>
#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace Fernanda {

using namespace Qt::StringLiterals;

// Help with punctuation while typing in the text editor: auto-pairing of
// brackets and quotes, and stepping past closing punctuation, each switched on
// or off alone.
//
// Auto-pairing:
// - An opener inserts its closer after the caret only before a space, the
//   line's end, or a closer, as Obsidian does. Before other text, which it's
//   likely to wrap, it types alone. ", ', and ` never pair after a letter or
//   digit ("don't", "5'")
// - An opener with text selected wraps the selection, which stays selected
// - Typing a closer that auto-pairing inserted, just after the caret, steps
//   over it; no other closer is stepped over. Inserted closers are tracked by
//   cursors, which follow edits, and dropped when the caret leaves their line
//   or the closer is gone
// - Backspace between an empty pair deletes both
//
// Stepping past (Hearth's "barging"):
// - A second space just before any closer moves past it: "(word |)" becomes
//   "(word) |"
// - Enter with only closers before the line's end moves past them first
// - Straight after a step, , . ? ! ; or : goes before the space ("(word), "),
//   as does more of it ("?!"), and Enter drops the space. Any other key, or a
//   caret move, ends this. ' and * aren't included, since they often start the
//   next word. That space is the one already typed, so a space typed after it
//   adds nothing
//
// Each change is one edit block, so it reaches the shared buffer as one edit
// and undoes as one step.
//
// Not a widget or an event filter: the editor owns it by value and offers it
// each key press first (TextEditor::keyPressEvent). Only a press that would
// type a character, or a plain Backspace or Enter, counts. Ctrl shortcuts
// don't, but Ctrl+Alt does: Windows reports AltGr that way, and it types
// brackets on many layouts
class TypingHelpers
{
public:
    explicit TypingHelpers(QPlainTextEdit* parentEditor)
        : editor_(parentEditor)
    {
    }

    [[nodiscard]] bool autoPairs() const noexcept { return autoPairs_; }

    void setAutoPairs(bool enabled)
    {
        if (enabled == autoPairs_) {
            return;
        }

        autoPairs_ = enabled;
        insertedClosers_.clear();
    }

    [[nodiscard]] bool barging() const noexcept { return barging_; }

    void setBarging(bool enabled)
    {
        if (enabled == barging_) {
            return;
        }

        barging_ = enabled;
        steppedTo_ = -1;
        closedGap_ = false;
    }

    // True means the key press was handled here, and the editor must not
    // handle it too
    bool keyPress(const QKeyEvent* event)
    {
        if (editor_->isReadOnly()) {
            return false;
        }

        auto has_selection = editor_->textCursor().hasSelection();

        if (event->key() == Qt::Key_Backspace) {
            return autoPairs_ && !has_selection &&
                   isPlainModifiers_(event, Qt::ShiftModifier) && deletePair_();
        }

        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            return barging_ && !has_selection &&
                   isPlainModifiers_(event, Qt::NoModifier) &&
                   (closeGapForEnter_() || stepPastClosersForEnter_());
        }

        if (!typesText_(event)) {
            return false;
        }

        auto character = event->text().at(0);

        // A space after a closed gap is the moved one, and is taken before
        // it can step past a closer: in "(word), " inside a quote, it's the
        // space after the comma, not the end of the quote
        if (barging_ && !has_selection &&
            (absorbSpace_(character) ||
             (character == u' ' && stepPastCloser_()) ||
             closeGap_(character))) {
            return true;
        }

        if (!autoPairs_) {
            return false;
        }

        if (has_selection) {
            return isOpener_(character) && wrapSelection_(character);
        }

        return stepOverCloser_(character) || insertPair_(character);
    }

    // The caret moved. A step past a closer is over, and inserted closers on
    // other lines are forgotten
    void caretMoved()
    {
        steppedTo_ = -1;
        closedGap_ = false;

        if (insertedClosers_.isEmpty()) {
            return;
        }

        auto block = editor_->textCursor().block();

        insertedClosers_.removeIf([&block](const InsertedCloser_& closer) {
            return closer.cursor.block() != block;
        });
    }

private:
    // A pair's two characters, which are the same for the quotes that open
    // and close alike
    struct Pair_
    {
        QChar opener;
        QChar closer;
    };

    // „ and ‚ are left out: which quote closes them depends on the language
    static constexpr Pair_ PAIRS_[] = {
        { u'(',      u')'      },
        { u'[',      u']'      },
        { u'{',      u'}'      },
        { u'"',      u'"'      },
        { u'\'',     u'\''     },
        { u'`',      u'`'      },
        { u'\u201C', u'\u201D' }, // “ ”
        { u'\u2018', u'\u2019' }, // ‘ ’
        { u'\u00AB', u'\u00BB' }, // « »
        { u'\u2039', u'\u203A' }, // ‹ ›
    };

    // An inserted closer: where it is, and which character it was, so one
    // since deleted or replaced isn't stepped over
    struct InsertedCloser_
    {
        QTextCursor cursor;
        QChar character;
    };

    QPlainTextEdit* editor_;
    bool autoPairs_ = false;
    bool barging_ = false;
    QList<InsertedCloser_> insertedClosers_{};

    // Where the caret was left by a step past a closer, or by punctuation
    // put before its space, while the next key can still close the gap; -1
    // otherwise. closedGap_ says it was punctuation, so a space typed next is
    // the one already there
    int steppedTo_ = -1;
    bool closedGap_ = false;

    [[nodiscard]] static const Pair_* pairOpenedBy_(QChar character)
    {
        for (const auto& pair : PAIRS_) {
            if (pair.opener == character) {
                return &pair;
            }
        }

        return nullptr;
    }

    [[nodiscard]] static bool isOpener_(QChar character)
    {
        return pairOpenedBy_(character) != nullptr;
    }

    [[nodiscard]] static bool isSymmetric_(QChar character)
    {
        auto* pair = pairOpenedBy_(character);
        return pair && pair->opener == pair->closer;
    }

    // Any pair's closer, including the quotes that open too
    [[nodiscard]] static bool isCloser_(QChar character)
    {
        for (const auto& pair : PAIRS_) {
            if (pair.closer == character) {
                return true;
            }
        }

        return false;
    }

    // Punctuation that goes before the space a step past a closer left
    [[nodiscard]] static bool isGapPunctuation_(QChar character)
    {
        return character == u',' || character == u'.' || character == u'?' ||
               character == u'!' || character == u';' || character == u':';
    }

    // A closer that isn't also an opener: ) ] } ” ’ » ›
    [[nodiscard]] static bool isOnlyCloser_(QChar character)
    {
        for (const auto& pair : PAIRS_) {
            if (pair.closer == character && pair.opener != character) {
                return true;
            }
        }

        return false;
    }

    // A key press Qt's editor would type as text: one printable character,
    // without Ctrl (or Ctrl+Shift) held, as QPlainTextEdit itself decides
    [[nodiscard]] static bool typesText_(const QKeyEvent* event)
    {
        auto text = event->text();

        if (text.size() != 1 || !text.at(0).isPrint()) {
            return false;
        }

        auto modifiers = event->modifiers() & ~Qt::KeypadModifier;

        return modifiers != Qt::ControlModifier &&
               modifiers != (Qt::ControlModifier | Qt::ShiftModifier);
    }

    // A key alone, or with only the one modifier allowed: Shift with
    // Backspace, which Qt treats the same as Backspace alone. Ctrl+Backspace
    // deletes a word, and Shift+Enter breaks the line within the paragraph,
    // and neither is for this
    [[nodiscard]] static bool
    isPlainModifiers_(const QKeyEvent* event, Qt::KeyboardModifier allowed)
    {
        auto modifiers = event->modifiers() & ~Qt::KeypadModifier;
        return modifiers == Qt::NoModifier || modifiers == allowed;
    }

    // The character at a document position in the caret's block, or a null
    // QChar past either end of it
    [[nodiscard]] static QChar
    characterInBlock_(const QTextCursor& cursor, int position)
    {
        auto block = cursor.block();
        auto in_block = position - block.position();

        if (in_block < 0 || in_block >= block.length() - 1) {
            return {};
        }

        return block.text().at(in_block);
    }

    // Typing an inserted closer that is just after the caret steps over it
    bool stepOverCloser_(QChar character)
    {
        auto cursor = editor_->textCursor();
        auto position = cursor.position();

        for (auto i = 0; i < insertedClosers_.size(); ++i) {
            const auto& closer = insertedClosers_.at(i);

            if (closer.cursor.position() != position ||
                closer.character != character ||
                characterInBlock_(cursor, position) != character) {
                continue;
            }

            insertedClosers_.removeAt(i);
            cursor.movePosition(QTextCursor::NextCharacter);
            editor_->setTextCursor(cursor);

            return true;
        }

        return false;
    }

    // An opener followed by its closer, with the caret between them, when the
    // rules in the class comment allow it
    bool insertPair_(QChar character)
    {
        auto* pair = pairOpenedBy_(character);

        if (!pair) {
            return false;
        }

        auto cursor = editor_->textCursor();
        auto position = cursor.position();

        auto next = characterInBlock_(cursor, position);

        if (!next.isNull() && !next.isSpace() && !isOnlyCloser_(next)) {
            return false;
        }

        if (isSymmetric_(character)) {
            auto previous = characterInBlock_(cursor, position - 1);

            if (previous.isLetterOrNumber()) {
                return false;
            }
        }

        cursor.beginEditBlock();
        cursor.insertText(QString(pair->opener) + pair->closer);
        cursor.endEditBlock();

        cursor.movePosition(QTextCursor::PreviousCharacter);
        editor_->setTextCursor(cursor);

        // A cursor at the closer stays before it as text is typed in front of
        // it, since text inserted at a cursor's position goes before it
        insertedClosers_.append({ cursor, pair->closer });

        return true;
    }

    // The selection, between the opener and its closer, still selected and
    // facing the way it did
    bool wrapSelection_(QChar character)
    {
        auto* pair = pairOpenedBy_(character);
        auto cursor = editor_->textCursor();

        auto start = cursor.selectionStart();
        auto end = cursor.selectionEnd();
        auto forward = cursor.position() == end;

        cursor.beginEditBlock();
        cursor.setPosition(end);
        cursor.insertText(QString(pair->closer));
        cursor.setPosition(start);
        cursor.insertText(QString(pair->opener));
        cursor.endEditBlock();

        cursor.setPosition(forward ? start + 1 : end + 1);
        cursor.setPosition(
            forward ? end + 1 : start + 1,
            QTextCursor::KeepAnchor);
        editor_->setTextCursor(cursor);

        return true;
    }

    // Backspace with the caret between an opener and its closer
    bool deletePair_()
    {
        auto cursor = editor_->textCursor();
        auto position = cursor.position();
        auto* pair = pairOpenedBy_(characterInBlock_(cursor, position - 1));

        if (!pair || characterInBlock_(cursor, position) != pair->closer) {
            return false;
        }

        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.deleteChar();
        cursor.endEditBlock();

        editor_->setTextCursor(cursor);
        return true;
    }

    // A second space, typed between a space and a closer: the space moves
    // past the closer, and the caret with it
    bool stepPastCloser_()
    {
        auto cursor = editor_->textCursor();
        auto position = cursor.position();

        if (characterInBlock_(cursor, position - 1) != u' ' ||
            !isCloser_(characterInBlock_(cursor, position))) {
            return false;
        }

        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.movePosition(QTextCursor::NextCharacter);
        cursor.insertText(u" "_s);
        cursor.endEditBlock();

        editor_->setTextCursor(cursor);

        // After setTextCursor, whose caretMoved would clear it
        steppedTo_ = cursor.position();
        closedGap_ = false;

        return true;
    }

    // Punctuation typed straight after a step, which goes before the space
    bool closeGap_(QChar character)
    {
        if (!isGapPunctuation_(character) || !isRightAfterStep_()) {
            return false;
        }

        auto cursor = editor_->textCursor();

        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.insertText(QString(character) + u' ');
        cursor.endEditBlock();

        editor_->setTextCursor(cursor);

        // After setTextCursor, whose caretMoved would clear them
        steppedTo_ = cursor.position();
        closedGap_ = true;

        return true;
    }

    // A space typed straight after a closed gap: the space is already there,
    // so nothing is added, and the chance is over
    bool absorbSpace_(QChar character)
    {
        if (character != u' ' || !closedGap_ || !isRightAfterStep_()) {
            return false;
        }

        steppedTo_ = -1;
        closedGap_ = false;

        return true;
    }

    // Enter straight after a step, which drops the space
    bool closeGapForEnter_()
    {
        if (!isRightAfterStep_()) {
            return false;
        }

        auto cursor = editor_->textCursor();

        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.insertBlock();
        cursor.endEditBlock();

        editor_->setTextCursor(cursor);
        return true;
    }

    // The caret is where a step (or a closed gap) left it, with the space it
    // put there before it. The text there can have changed since without the
    // caret moving (an edit in another view of the file), hence the space
    [[nodiscard]] bool isRightAfterStep_() const
    {
        auto cursor = editor_->textCursor();
        auto position = cursor.position();

        return position == steppedTo_ &&
               characterInBlock_(cursor, position - 1) == u' ';
    }

    // Enter with only closers between the caret and the end of its line:
    // the caret moves past them, and the line breaks there
    bool stepPastClosersForEnter_()
    {
        auto cursor = editor_->textCursor();
        auto rest = cursor.block().text().mid(cursor.positionInBlock());

        if (rest.isEmpty()) {
            return false;
        }

        for (auto character : rest) {
            if (!isCloser_(character)) {
                return false;
            }
        }

        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertBlock();
        cursor.endEditBlock();

        editor_->setTextCursor(cursor);
        return true;
    }
};

} // namespace Fernanda
