/*
 * Suzuri — A plain-text editor for creative writing
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
#include <QKeyEvent>
#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace Suzuri {

using namespace Qt::StringLiterals;

// Help with punctuation while typing in the text editor: auto-pairing of
// brackets and quotes, and stepping past closing punctuation. Each can be
// turned on or off alone.
//
// Auto-pairing:
//
// - Typing an opener inserts its closer after the caret, when what follows
//   the caret is a space, the end of the line, or a closer (as CodeMirror,
//   and so Obsidian, decides). Before other text it inserts the opener alone,
//   since that is usually the start of wrapping text already there
// - The quotes that open and close with one character (", ', `) never pair
//   after a letter or digit, where they're an apostrophe or a closing quote
//   ("don't", "5'")
// - Typing an opener with text selected wraps the selection, which stays
//   selected
// - Typing a closer that auto-pairing inserted, when it's just after the
//   caret, steps over it. A closer the user typed, or one already in the
//   text, is never stepped over. The inserted closers are remembered by
//   cursors, which keep their places as the text changes, and forgotten when
//   the caret leaves their line or the closer is gone
// - Backspace between an empty pair deletes both
//
// Stepping past closing punctuation (Hearth's "barging"), for finishing a
// bracket or quote without reaching for the closer:
//
// - A second space typed just before a closer moves past it: "(word |)"
//   becomes "(word) |". The closer can be any pair's, typed or inserted
// - Enter, with nothing but closers between the caret and the end of the
//   line, moves past them before starting the next line, so a paragraph that
//   ends inside a bracket or quote ends after it. Elsewhere, Enter is left
//   alone
// - Straight after a step, with the caret still where it left it, typing
//   , . ? ! ; or : goes before the space: "(word) " and a comma become
//   "(word), ". Enter there drops the space. Any other key, or moving the
//   caret, ends the chance. ' and * aren't among them, since they so often
//   start the next word
//
// Each change is one edit block in the view's document, so it reaches the
// shared buffer as one edit and undoes as one step.
//
// Not a widget or an event filter: the editor owns it by value and offers it
// each key press first (TextEditor::keyPressEvent), as it does the mouse to
// SelectionHandles. Only a key press that would type a character, or a plain
// Backspace or Enter, is considered: one Qt itself wouldn't type (a Ctrl
// shortcut) is left alone. Ctrl+Alt is how Windows reports AltGr, which types
// brackets on many layouts, so it is allowed
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

        if (barging_ && !has_selection &&
            ((character == u' ' && stepPastCloser_()) ||
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

    // Where the caret was left by a step past a closer, while the next key
    // can still close the gap; -1 otherwise
    int steppedTo_ = -1;

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

    // The caret is where a step left it, with the space it put there before
    // it. The text there can have changed since without the caret moving (an
    // edit in another view of the file), hence the space
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

} // namespace Suzuri
