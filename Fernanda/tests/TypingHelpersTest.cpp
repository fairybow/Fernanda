/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <QKeyEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QString>
#include <QTest>
#include <QTextBlock>
#include <QTextCursor>

#include "views/TypingHelpers.h"

using namespace Qt::StringLiterals;
using Fernanda::TypingHelpers;

// Auto-pairing and stepping past closers (views/TypingHelpers.h), typed into a
// real QPlainTextEdit that offers its key presses to the helpers first and
// tells them when the caret moves, as TextEditor does. Each test checks the
// text and where the caret (or the selection) ends up.
//
// Qt Test runs every private slot as a test
class TypingHelpersTest : public QObject
{
    Q_OBJECT

private:
    class Editor_ : public QPlainTextEdit
    {
    public:
        Editor_()
        {
            helpers.setAutoPairs(true);
            helpers.setBarging(true);
            connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
                helpers.caretMoved();
            });
        }

        TypingHelpers helpers{ this };

    protected:
        void keyPressEvent(QKeyEvent* event) override
        {
            if (helpers.keyPress(event)) {
                event->accept();
                return;
            }

            QPlainTextEdit::keyPressEvent(event);
        }
    };

    // Type each character as a key press carrying it as text, as a keyboard
    // does. Unicode is typed this way too: QTest::keyClicks takes Latin-1
    static void type_(
        QPlainTextEdit& editor,
        const QString& text,
        Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        for (auto character : text) {
            QTest::sendKeyEvent(
                QTest::Click,
                &editor,
                Qt::Key_unknown,
                QString(character),
                modifiers);
        }
    }

    static void backspace_(QPlainTextEdit& editor)
    {
        QTest::keyClick(&editor, Qt::Key_Backspace);
    }

    static void enter_(QPlainTextEdit& editor)
    {
        QTest::keyClick(&editor, Qt::Key_Return);
    }

    // Text, with the caret at caret
    static void set_(QPlainTextEdit& editor, const QString& text, int caret)
    {
        editor.setPlainText(text);

        auto cursor = editor.textCursor();
        cursor.setPosition(caret);
        editor.setTextCursor(cursor);
    }

    static void select_(QPlainTextEdit& editor, int anchor, int position)
    {
        auto cursor = editor.textCursor();
        cursor.setPosition(anchor);
        cursor.setPosition(position, QTextCursor::KeepAnchor);
        editor.setTextCursor(cursor);
    }

    [[nodiscard]] static int caret_(const QPlainTextEdit& editor)
    {
        return editor.textCursor().position();
    }

private slots:
    // --- Pairing ---------------------------------------------------------

    void anOpenerAtTheEndOfALinePairs()
    {
        Editor_ editor{};
        type_(editor, u"("_s);

        QCOMPARE(editor.toPlainText(), u"()"_s);
        QCOMPARE(caret_(editor), 1);
    }

    void anOpenerBeforeASpacePairs()
    {
        Editor_ editor{};
        set_(editor, u"a b"_s, 1);
        type_(editor, u"("_s);

        QCOMPARE(editor.toPlainText(), u"a() b"_s);
        QCOMPARE(caret_(editor), 2);
    }

    void anOpenerBeforeACloserPairs()
    {
        Editor_ editor{};
        type_(editor, u"(["_s);

        QCOMPARE(editor.toPlainText(), u"([])"_s);
        QCOMPARE(caret_(editor), 2);
    }

    void anOpenerBeforeAWordDoesNotPair()
    {
        Editor_ editor{};
        set_(editor, u"word"_s, 0);
        type_(editor, u"("_s);

        QCOMPARE(editor.toPlainText(), u"(word"_s);
        QCOMPARE(caret_(editor), 1);
    }

    void anOpenerBeforePunctuationDoesNotPair()
    {
        Editor_ editor{};
        set_(editor, u"."_s, 0);
        type_(editor, u"("_s);

        QCOMPARE(editor.toPlainText(), u"(."_s);
    }

    void aQuoteAfterALetterDoesNotPair()
    {
        Editor_ editor{};
        type_(editor, u"don't"_s);

        QCOMPARE(editor.toPlainText(), u"don't"_s);
    }

    void aQuoteAfterADigitDoesNotPair()
    {
        Editor_ editor{};
        type_(editor, u"5'"_s);

        QCOMPARE(editor.toPlainText(), u"5'"_s);
    }

    void aQuoteAfterASpacePairs()
    {
        Editor_ editor{};
        type_(editor, u"say \""_s);

        QCOMPARE(editor.toPlainText(), u"say \"\""_s);
        QCOMPARE(caret_(editor), 5);
    }

    // The closer is the pair's, not the opener again
    void aCurlyQuotePairsWithItsCloser()
    {
        Editor_ editor{};
        type_(editor, u"\u201C"_s);

        QCOMPARE(editor.toPlainText(), u"\u201C\u201D"_s);
    }

    void aGuillemetPairsWithItsCloser()
    {
        Editor_ editor{};
        type_(editor, u"\u00AB"_s);

        QCOMPARE(editor.toPlainText(), u"\u00AB\u00BB"_s);
    }

    // A low quote (U+201E) closes differently from language to language
    void aLowQuoteDoesNotPair()
    {
        Editor_ editor{};
        type_(editor, u"\u201E"_s);

        QCOMPARE(editor.toPlainText(), u"\u201E"_s);
    }

    void aPairUndoesAsOneStep()
    {
        Editor_ editor{};
        type_(editor, u"a ("_s);
        editor.undo();

        QCOMPARE(editor.toPlainText(), u"a "_s);
    }

    // --- Stepping over closers ---------------------------------------------

    void typingAnInsertedCloserStepsOverIt()
    {
        Editor_ editor{};
        type_(editor, u"(word)"_s);

        QCOMPARE(editor.toPlainText(), u"(word)"_s);
        QCOMPARE(caret_(editor), 6);
    }

    void typingInsertedQuotesStepsOverThem()
    {
        Editor_ editor{};
        type_(editor, u"\"hi\""_s);

        QCOMPARE(editor.toPlainText(), u"\"hi\""_s);
        QCOMPARE(caret_(editor), 4);
    }

    void nestedPairsAreSteppedOverInTurn()
    {
        Editor_ editor{};
        type_(editor, u"([a])"_s);

        QCOMPARE(editor.toPlainText(), u"([a])"_s);
        QCOMPARE(caret_(editor), 5);
    }

    // A closer already in the text is the user's, and typing one before it
    // adds another
    void aCloserNotInsertedIsNotSteppedOver()
    {
        Editor_ editor{};
        set_(editor, u"(a)"_s, 2);
        type_(editor, u")"_s);

        QCOMPARE(editor.toPlainText(), u"(a))"_s);
        QCOMPARE(caret_(editor), 3);
    }

    void aCloserIsForgottenOnceTheCaretLeavesItsLine()
    {
        Editor_ editor{};
        set_(editor, u"\nnext"_s, 0);
        type_(editor, u"("_s);

        auto cursor = editor.textCursor();
        cursor.movePosition(QTextCursor::Down);
        editor.setTextCursor(cursor);
        cursor.setPosition(1);
        editor.setTextCursor(cursor);

        type_(editor, u")"_s);

        QCOMPARE(editor.toPlainText(), u"())\nnext"_s);
    }

    void aCloserIsKeptWhileTheCaretStaysOnItsLine()
    {
        Editor_ editor{};
        type_(editor, u"(ab"_s);

        auto cursor = editor.textCursor();
        cursor.movePosition(QTextCursor::Left);
        cursor.movePosition(QTextCursor::Right);
        editor.setTextCursor(cursor);

        type_(editor, u")"_s);

        QCOMPARE(editor.toPlainText(), u"(ab)"_s);
    }

    // --- Wrapping a selection ----------------------------------------------

    void anOpenerWrapsASelection()
    {
        Editor_ editor{};
        set_(editor, u"one two"_s, 0);
        select_(editor, 4, 7);
        type_(editor, u"("_s);

        QCOMPARE(editor.toPlainText(), u"one (two)"_s);
        QCOMPARE(editor.textCursor().selectedText(), u"two"_s);
        QCOMPARE(editor.textCursor().anchor(), 5);
        QCOMPARE(caret_(editor), 8);
    }

    void aWrappedSelectionKeepsItsDirection()
    {
        Editor_ editor{};
        set_(editor, u"one two"_s, 0);
        select_(editor, 7, 4);
        type_(editor, u"["_s);

        QCOMPARE(editor.toPlainText(), u"one [two]"_s);
        QCOMPARE(editor.textCursor().anchor(), 8);
        QCOMPARE(caret_(editor), 5);
    }

    // A selection is wrapped whatever is around it, a quote after a letter
    // included
    void aQuoteWrapsASelectionAfterALetter()
    {
        Editor_ editor{};
        set_(editor, u"onetwo"_s, 0);
        select_(editor, 3, 6);
        type_(editor, u"\""_s);

        QCOMPARE(editor.toPlainText(), u"one\"two\""_s);
    }

    void aCloserReplacesASelection()
    {
        Editor_ editor{};
        set_(editor, u"one two"_s, 0);
        select_(editor, 4, 7);
        type_(editor, u")"_s);

        QCOMPARE(editor.toPlainText(), u"one )"_s);
    }

    void aWrapUndoesAsOneStep()
    {
        Editor_ editor{};
        set_(editor, u"one two"_s, 0);
        select_(editor, 4, 7);
        type_(editor, u"("_s);
        editor.undo();

        QCOMPARE(editor.toPlainText(), u"one two"_s);
    }

    // --- Deleting a pair ---------------------------------------------------

    void backspaceBetweenAnEmptyPairDeletesBoth()
    {
        Editor_ editor{};
        set_(editor, u"a ()"_s, 3);
        backspace_(editor);

        QCOMPARE(editor.toPlainText(), u"a "_s);
        QCOMPARE(caret_(editor), 2);
    }

    void backspaceAfterAnOpenerWithTextAfterItDeletesOne()
    {
        Editor_ editor{};
        set_(editor, u"(a)"_s, 1);
        backspace_(editor);

        QCOMPARE(editor.toPlainText(), u"a)"_s);
    }

    void backspaceBetweenMismatchedCharactersDeletesOne()
    {
        Editor_ editor{};
        set_(editor, u"(]"_s, 1);
        backspace_(editor);

        QCOMPARE(editor.toPlainText(), u"]"_s);
    }

    // --- Stepping past closers ---------------------------------------------

    void aSecondSpaceBeforeACloserStepsPastIt()
    {
        Editor_ editor{};
        type_(editor, u"(word  "_s);

        QCOMPARE(editor.toPlainText(), u"(word) "_s);
        QCOMPARE(caret_(editor), 7);
    }

    void oneSpaceBeforeACloserIsJustASpace()
    {
        Editor_ editor{};
        type_(editor, u"(word "_s);

        QCOMPARE(editor.toPlainText(), u"(word )"_s);
        QCOMPARE(caret_(editor), 6);
    }

    void aCloserAlreadyInTheTextIsSteppedPast()
    {
        Editor_ editor{};
        set_(editor, u"(word) more"_s, 5);
        type_(editor, u"  "_s);

        QCOMPARE(editor.toPlainText(), u"(word)  more"_s);
        QCOMPARE(caret_(editor), 7);
    }

    void eachSecondSpaceStepsPastOneCloser()
    {
        Editor_ editor{};
        type_(editor, u"(\"a   "_s);

        QCOMPARE(editor.toPlainText(), u"(\"a\") "_s);
        QCOMPARE(caret_(editor), 6);
    }

    void stepsWorkWithAutoPairingOff()
    {
        Editor_ editor{};
        editor.helpers.setAutoPairs(false);
        set_(editor, u"(word)"_s, 5);
        type_(editor, u"  "_s);

        QCOMPARE(editor.toPlainText(), u"(word) "_s);
    }

    void aStepUndoesAsOneStep()
    {
        Editor_ editor{};
        set_(editor, u"(word )"_s, 6);
        type_(editor, u" "_s);
        editor.undo();

        QCOMPARE(editor.toPlainText(), u"(word )"_s);
    }

    void punctuationRightAfterAStepGoesBeforeTheSpace_data()
    {
        QTest::addColumn<QString>("punctuation");

        for (const auto& mark :
             { u","_s, u"."_s, u"?"_s, u"!"_s, u";"_s, u":"_s }) {
            QTest::newRow(qPrintable(mark)) << mark;
        }
    }

    void punctuationRightAfterAStepGoesBeforeTheSpace()
    {
        QFETCH(QString, punctuation);

        Editor_ editor{};
        type_(editor, u"(word  "_s + punctuation);

        QCOMPARE(editor.toPlainText(), u"(word)"_s + punctuation + u" "_s);
        QCOMPARE(caret_(editor), 8);
    }

    // ' and * so often start the next word
    void anAsteriskAfterAStepStaysAfterTheSpace()
    {
        Editor_ editor{};
        type_(editor, u"(word  *"_s);

        QCOMPARE(editor.toPlainText(), u"(word) *"_s);
    }

    void anApostropheAfterAStepStaysAfterTheSpace()
    {
        Editor_ editor{};
        editor.helpers.setAutoPairs(false);
        set_(editor, u"(word)"_s, 5);
        type_(editor, u"  '"_s);

        QCOMPARE(editor.toPlainText(), u"(word) '"_s);
    }

    void punctuationAfterAnotherKeyStaysPut()
    {
        Editor_ editor{};
        type_(editor, u"(word  a,"_s);

        QCOMPARE(editor.toPlainText(), u"(word) a,"_s);
    }

    void punctuationAfterTheCaretMovesStaysPut()
    {
        Editor_ editor{};
        type_(editor, u"(word  "_s);

        auto cursor = editor.textCursor();
        cursor.movePosition(QTextCursor::Left);
        editor.setTextCursor(cursor);
        cursor.movePosition(QTextCursor::Right);
        editor.setTextCursor(cursor);

        type_(editor, u","_s);

        QCOMPARE(editor.toPlainText(), u"(word) ,"_s);
    }

    // Punctuation after a space and a closer the user typed is theirs
    void punctuationAfterATypedSpaceStaysPut()
    {
        Editor_ editor{};
        set_(editor, u"(word) "_s, 7);
        type_(editor, u","_s);

        QCOMPARE(editor.toPlainText(), u"(word) ,"_s);
    }

    void aSpaceAfterAClosedGapIsTheOneAlreadyThere()
    {
        Editor_ editor{};
        type_(editor, u"(word  , then"_s);

        QCOMPARE(editor.toPlainText(), u"(word), then"_s);
    }

    void aWordAfterAClosedGapKeepsTheSpace()
    {
        Editor_ editor{};
        type_(editor, u"(word  ,then"_s);

        QCOMPARE(editor.toPlainText(), u"(word), then"_s);
    }

    void onlyOneSpaceAfterAClosedGapIsTaken()
    {
        Editor_ editor{};
        type_(editor, u"(word  ,  "_s);

        QCOMPARE(editor.toPlainText(), u"(word),  "_s);
    }

    void aSpaceAfterAClosedGapIsTypedOnceTheCaretMoves()
    {
        Editor_ editor{};
        type_(editor, u"(word  ,"_s);

        auto cursor = editor.textCursor();
        cursor.movePosition(QTextCursor::Left);
        editor.setTextCursor(cursor);
        cursor.movePosition(QTextCursor::Right);
        editor.setTextCursor(cursor);

        type_(editor, u" "_s);

        QCOMPARE(editor.toPlainText(), u"(word),  "_s);
    }

    // Inside a quote, the space belongs after the comma rather than ending
    // the quote
    void aSpaceAfterAClosedGapDoesNotStepPastACloser()
    {
        Editor_ editor{};
        set_(editor, u"\"(word)\""_s, 6);
        type_(editor, u"  , "_s);

        QCOMPARE(editor.toPlainText(), u"\"(word), \""_s);
        QCOMPARE(caret_(editor), 9);
    }

    void moreMarksAfterAClosedGapGoBeforeTheSpace()
    {
        Editor_ editor{};
        type_(editor, u"(word  ?! "_s);

        QCOMPARE(editor.toPlainText(), u"(word)?! "_s);
        QCOMPARE(caret_(editor), 9);
    }

    void enterAfterAClosedGapDropsTheSpace()
    {
        Editor_ editor{};
        type_(editor, u"(word  ."_s);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"(word).\n"_s);
        QCOMPARE(caret_(editor), 8);
    }

    void aSpaceTakenAfterAClosedGapLeavesNothingToUndo()
    {
        Editor_ editor{};
        type_(editor, u"(word  , "_s);
        editor.undo();

        QCOMPARE(editor.toPlainText(), u"(word) "_s);
    }

    void enterRightAfterAStepDropsTheSpace()
    {
        Editor_ editor{};
        type_(editor, u"(word  "_s);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"(word)\n"_s);
        QCOMPARE(caret_(editor), 7);
    }

    void enterBeforeClosersEndingALineStepsPastThem()
    {
        Editor_ editor{};
        type_(editor, u"(\"a"_s);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"(\"a\")\n"_s);
        QCOMPARE(caret_(editor), 6);
    }

    void enterBeforeACloserMidLineIsLeftAlone()
    {
        Editor_ editor{};
        set_(editor, u"(a) b"_s, 2);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"(a\n) b"_s);
    }

    void enterAtTheEndOfALineIsLeftAlone()
    {
        Editor_ editor{};
        set_(editor, u"abc"_s, 3);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"abc\n"_s);
    }

    void nothingStepsWhenOff()
    {
        Editor_ editor{};
        editor.helpers.setBarging(false);
        type_(editor, u"(word  "_s);
        enter_(editor);

        QCOMPARE(editor.toPlainText(), u"(word  \n)"_s);
    }

    // --- When the helpers stay out of it ------------------------------------

    void nothingPairsWhenOff()
    {
        Editor_ editor{};
        editor.helpers.setAutoPairs(false);
        type_(editor, u"("_s);
        backspace_(editor);
        type_(editor, u"(\""_s);

        QCOMPARE(editor.toPlainText(), u"(\""_s);
    }

    // Ctrl with a character is a shortcut, which Qt doesn't type either
    void aControlShortcutIsLeftAlone()
    {
        Editor_ editor{};
        type_(editor, u"("_s, Qt::ControlModifier);

        QCOMPARE(editor.toPlainText(), QString{});
    }

    // Windows reports AltGr, which types brackets on many layouts, as
    // Ctrl+Alt
    void aBracketTypedWithAltGrPairs()
    {
        Editor_ editor{};
        type_(editor, u"["_s, Qt::ControlModifier | Qt::AltModifier);

        QCOMPARE(editor.toPlainText(), u"[]"_s);
    }

    void aReadOnlyEditorIsLeftAlone()
    {
        Editor_ editor{};
        set_(editor, u"()"_s, 1);
        editor.setReadOnly(true);
        backspace_(editor);
        type_(editor, u"["_s);

        QCOMPARE(editor.toPlainText(), u"()"_s);
    }
};

QTEST_MAIN(TypingHelpersTest)

#include "TypingHelpersTest.moc"
