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

#include <functional>

#include <QChar>
#include <QMetaType>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include "models/PrimeDocument.h"

using namespace Qt::StringLiterals;
using Suzuri::PrimeDocument;

// An edit as a user makes one: something done with a cursor on a view's
// document
using Edit = std::function<void(QTextCursor&)>;

// Lets an Edit sit in a test table's column
Q_DECLARE_METATYPE(Edit)

// How a prime document keeps its views in step (models/PrimeDocument.h): an
// edit made in any view must leave the prime and every view holding the same
// text, and undo must take them all back together.
//
// Text outside ASCII is built from code point numbers, so nothing here
// depends on how the compiler reads this file.
//
// PrimeDocument checks for drift itself after every edit, and resets a view
// that has drifted from the prime. A case that makes a view drift therefore
// fails below only if the prime was left holding the wrong text.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class PrimeDocumentTest : public QObject
{
    Q_OBJECT

private:
    // A document's text as PrimeDocument reads it: nothing substituted, line
    // breaks as '\n'
    [[nodiscard]] static QString textOf_(const QTextDocument& document)
    {
        auto text = document.toRawText();
        text.replace(QChar::ParagraphSeparator, QChar(u'\n'));
        return text;
    }

    // A view's document, set up as TextFileView sets its own up
    static void setUpView_(QTextDocument& document, PrimeDocument& prime)
    {
        document.setDocumentLayout(new QPlainTextDocumentLayout(&document));
        prime.registerView(&document);
    }

    // A prime holding some text, with two views on it. The views are declared
    // after the prime so they are destroyed first, as a view is in the app
    struct Fixture_
    {
        explicit Fixture_(const QString& text)
        {
            prime.setText(text);
            setUpView_(a, prime);
            setUpView_(b, prime);
        }

        PrimeDocument prime{ nullptr };
        QTextDocument a{};
        QTextDocument b{};
    };

// QVERIFY and QCOMPARE return from the function they are in, so a check
// shared by many tests has to be a macro to stop the test that uses it
#define VERIFY_ALL_HOLD(fixture, expected)                                     \
    do {                                                                       \
        QCOMPARE((fixture).prime.text(), (expected));                          \
        QCOMPARE(textOf_((fixture).a), (expected));                            \
        QCOMPARE(textOf_((fixture).b), (expected));                            \
    } while (false)

    [[nodiscard]] static QString noBreakSpace_() { return QChar(0x00A0); }
    [[nodiscard]] static QString lineSeparator_() { return QChar(0x2028); }

    // Outside the BMP, so two UTF-16 code units and two cursor positions
    [[nodiscard]] static QString emoji_()
    {
        return QString::fromUcs4(U"\x1F4C1", 1);
    }

private slots:
    // --- Registering --------------------------------------------------------

    void registerViewSeedsTheView()
    {
        PrimeDocument prime(nullptr);
        prime.setText(u"one\ntwo"_s);

        QTextDocument view{};
        view.setPlainText(u"something else"_s);
        setUpView_(view, prime);

        QCOMPARE(textOf_(view), u"one\ntwo"_s);
        QCOMPARE(prime.viewCount(), 1);

        // Seeding a view is not an edit
        QVERIFY(!prime.isUndoAvailable());
    }

    // Undo lives on the prime alone
    void registerViewDisablesTheViewsOwnUndo()
    {
        Fixture_ f(u"one"_s);

        QVERIFY(!f.a.isUndoRedoEnabled());
        QVERIFY(!f.b.isUndoRedoEnabled());
    }

    void registerViewTwiceCountsOnce()
    {
        Fixture_ f(u"one"_s);
        f.prime.registerView(&f.a);

        QCOMPARE(f.prime.viewCount(), 2);

        QTextCursor cursor(&f.a);
        cursor.insertText(u"X"_s);

        VERIFY_ALL_HOLD(f, u"Xone"_s);
    }

    // --- One edit in one view -----------------------------------------------

    // Each row is some text, one edit made in a view, and the text that should
    // result. The cursor starts at the beginning of the document
    void editInOneView_data()
    {
        QTest::addColumn<QString>("initial");
        QTest::addColumn<Edit>("edit");
        QTest::addColumn<QString>("expected");

        // Typing
        QTest::newRow("type at the start")
            << u"one\ntwo"_s
            << Edit([](QTextCursor& c) { c.insertText(u"X"_s); })
            << u"Xone\ntwo"_s;

        QTest::newRow("type in the middle")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.setPosition(2);
                   c.insertText(u"X"_s);
               })
            << u"onXe\ntwo"_s;

        QTest::newRow("type at the end")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.movePosition(QTextCursor::End);
                   c.insertText(u"X"_s);
               })
            << u"one\ntwoX"_s;

        QTest::newRow("type into an empty document")
            << u""_s << Edit([](QTextCursor& c) { c.insertText(u"X"_s); })
            << u"X"_s;

        QTest::newRow("type after a final line break")
            << u"one\n"_s << Edit([](QTextCursor& c) {
                   c.movePosition(QTextCursor::End);
                   c.insertText(u"X"_s);
               })
            << u"one\nX"_s;

        QTest::newRow("type a tab")
            << u"one"_s << Edit([](QTextCursor& c) { c.insertText(u"\t"_s); })
            << u"\tone"_s;

        // Line breaks
        QTest::newRow("Enter in the middle of a line")
            << u"onetwo"_s << Edit([](QTextCursor& c) {
                   c.setPosition(3);
                   c.insertBlock();
               })
            << u"one\ntwo"_s;

        QTest::newRow("Enter at the end")
            << u"one"_s << Edit([](QTextCursor& c) {
                   c.movePosition(QTextCursor::End);
                   c.insertBlock();
               })
            << u"one\n"_s;

        QTest::newRow("Enter at the start")
            << u"one"_s << Edit([](QTextCursor& c) { c.insertBlock(); })
            << u"\none"_s;

        QTest::newRow("Enter in an empty document")
            << u""_s << Edit([](QTextCursor& c) { c.insertBlock(); })
            << u"\n"_s;

        // Pasting
        QTest::newRow("paste several lines into a line")
            << u"xy"_s << Edit([](QTextCursor& c) {
                   c.setPosition(1);
                   c.insertText(u"A\nB\nC"_s);
               })
            << u"xA\nB\nCy"_s;

        QTest::newRow("paste text ending in a line break")
            << u"xy"_s << Edit([](QTextCursor& c) {
                   c.setPosition(1);
                   c.insertText(u"A\n"_s);
               })
            << u"xA\ny"_s;

        QTest::newRow("paste only line breaks")
            << u"xy"_s << Edit([](QTextCursor& c) {
                   c.setPosition(1);
                   c.insertText(u"\n\n"_s);
               })
            << u"x\n\ny"_s;

        // A pasted CRLF is one break, as the document reads it
        QTest::newRow("paste a CRLF") << u"xy"_s << Edit([](QTextCursor& c) {
            c.setPosition(1);
            c.insertText(u"A\r\nB"_s);
        }) << u"xA\nBy"_s;

        // Deleting
        QTest::newRow("Backspace") << u"one"_s << Edit([](QTextCursor& c) {
            c.movePosition(QTextCursor::End);
            c.deletePreviousChar();
        }) << u"on"_s;

        QTest::newRow("Delete")
            << u"one"_s << Edit([](QTextCursor& c) { c.deleteChar(); })
            << u"ne"_s;

        QTest::newRow("delete the only character")
            << u"x"_s << Edit([](QTextCursor& c) { c.deleteChar(); }) << u""_s;

        QTest::newRow("Backspace joins two lines")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.setPosition(4);
                   c.deletePreviousChar();
               })
            << u"onetwo"_s;

        QTest::newRow("Backspace removes a final line break")
            << u"one\n"_s << Edit([](QTextCursor& c) {
                   c.movePosition(QTextCursor::End);
                   c.deletePreviousChar();
               })
            << u"one"_s;

        QTest::newRow("delete a selection in a line")
            << u"one two"_s << Edit([](QTextCursor& c) {
                   c.setPosition(3);
                   c.setPosition(7, QTextCursor::KeepAnchor);
                   c.removeSelectedText();
               })
            << u"one"_s;

        QTest::newRow("delete a selection across lines")
            << u"one\ntwo\nthree"_s << Edit([](QTextCursor& c) {
                   c.setPosition(2);
                   c.setPosition(9, QTextCursor::KeepAnchor);
                   c.removeSelectedText();
               })
            << u"onhree"_s;

        QTest::newRow("delete a selection made backward")
            << u"one two"_s << Edit([](QTextCursor& c) {
                   c.setPosition(7);
                   c.setPosition(3, QTextCursor::KeepAnchor);
                   c.removeSelectedText();
               })
            << u"one"_s;

        // Typing over a selection
        QTest::newRow("type over a selection")
            << u"one two"_s << Edit([](QTextCursor& c) {
                   c.setPosition(0);
                   c.setPosition(3, QTextCursor::KeepAnchor);
                   c.insertText(u"1"_s);
               })
            << u"1 two"_s;

        QTest::newRow("paste lines over a selection across lines")
            << u"one\ntwo\nthree"_s << Edit([](QTextCursor& c) {
                   c.setPosition(2);
                   c.setPosition(9, QTextCursor::KeepAnchor);
                   c.insertText(u"A\nB"_s);
               })
            << u"onA\nBhree"_s;

        // The whole document
        QTest::newRow("select all and type")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.select(QTextCursor::Document);
                   c.insertText(u"X"_s);
               })
            << u"X"_s;

        QTest::newRow("select all and paste lines")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.select(QTextCursor::Document);
                   c.insertText(u"A\nB\nC"_s);
               })
            << u"A\nB\nC"_s;

        QTest::newRow("select all and delete")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.select(QTextCursor::Document);
                   c.removeSelectedText();
               })
            << u""_s;

        QTest::newRow("select all and type, with a final line break")
            << u"one\n"_s << Edit([](QTextCursor& c) {
                   c.select(QTextCursor::Document);
                   c.insertText(u"X"_s);
               })
            << u"X"_s;

        QTest::newRow("select all and paste the same text")
            << u"one\ntwo"_s << Edit([](QTextCursor& c) {
                   c.select(QTextCursor::Document);
                   c.insertText(u"one\ntwo"_s);
               })
            << u"one\ntwo"_s;

        // Several changes the view reports as one
        QTest::newRow("two edits far apart, as one step")
            << u"one\ntwo\nthree"_s << Edit([](QTextCursor& c) {
                   c.beginEditBlock();
                   c.insertText(u"A"_s);
                   c.movePosition(QTextCursor::End);
                   c.insertText(u"Z"_s);
                   c.endEditBlock();
               })
            << u"Aone\ntwo\nthreeZ"_s;

        QTest::newRow("move text within a line, as one step")
            << u"one two"_s << Edit([](QTextCursor& c) {
                   c.beginEditBlock();
                   c.setPosition(0);
                   c.setPosition(4, QTextCursor::KeepAnchor);
                   c.removeSelectedText();
                   c.movePosition(QTextCursor::End);
                   c.insertText(u" one"_s);
                   c.endEditBlock();
               })
            << u"two one"_s;

        QTest::newRow("a deletion and an insertion on other lines, as one step")
            << u"one\ntwo\nthree"_s << Edit([](QTextCursor& c) {
                   c.beginEditBlock();
                   c.setPosition(0);
                   c.setPosition(4, QTextCursor::KeepAnchor);
                   c.removeSelectedText();
                   c.movePosition(QTextCursor::End);
                   c.insertText(u"\nfour"_s);
                   c.endEditBlock();
               })
            << u"two\nthree\nfour"_s;

        // Characters a lossy path would change
        QTest::newRow("type a no-break space")
            << u"ab"_s << Edit([](QTextCursor& c) {
                   c.setPosition(1);
                   c.insertText(noBreakSpace_());
               })
            << u"a"_s + noBreakSpace_() + u"b"_s;

        QTest::newRow("type a line separator")
            << u"ab"_s << Edit([](QTextCursor& c) {
                   c.setPosition(1);
                   c.insertText(lineSeparator_());
               })
            << u"a"_s + lineSeparator_() + u"b"_s;

        QTest::newRow("type an emoji") << u"ab"_s << Edit([](QTextCursor& c) {
            c.setPosition(1);
            c.insertText(emoji_());
        }) << u"a"_s + emoji_() + u"b"_s;

        QTest::newRow("Backspace over an emoji")
            << u"a"_s + emoji_() + u"b"_s << Edit([](QTextCursor& c) {
                   c.setPosition(3);
                   c.deletePreviousChar();
               })
            << u"ab"_s;
    }

    void editInOneView()
    {
        QFETCH(QString, initial);
        QFETCH(Edit, edit);
        QFETCH(QString, expected);

        Fixture_ f(initial);

        QTextCursor cursor(&f.a);
        edit(cursor);

        VERIFY_ALL_HOLD(f, expected);

        // The edit is one step on the prime's stack, and undoing it takes
        // every view back with it
        if (initial != expected) {
            QVERIFY(f.prime.isUndoAvailable());

            f.prime.undo();
            VERIFY_ALL_HOLD(f, initial);

            f.prime.redo();
            VERIFY_ALL_HOLD(f, expected);
        }
    }

    // --- Edits from more than one view --------------------------------------

    void editsFromTwoViewsInterleave()
    {
        Fixture_ f(u"one\ntwo"_s);

        QTextCursor in_a(&f.a);
        QTextCursor in_b(&f.b);

        in_a.insertText(u"A"_s);
        VERIFY_ALL_HOLD(f, u"Aone\ntwo"_s);

        in_b.movePosition(QTextCursor::End);
        in_b.insertText(u"B"_s);
        VERIFY_ALL_HOLD(f, u"Aone\ntwoB"_s);

        // A cursor in one view keeps its place across the other view's edit
        in_a.insertText(u"C"_s);
        VERIFY_ALL_HOLD(f, u"ACone\ntwoB"_s);

        // One stack: undo runs back through both views' edits, newest first
        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"Aone\ntwoB"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"Aone\ntwo"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one\ntwo"_s);

        QVERIFY(!f.prime.isUndoAvailable());
    }

    void threeViewsStayInStep()
    {
        Fixture_ f(u"one"_s);

        QTextDocument c{};
        setUpView_(c, f.prime);

        QTextCursor cursor(&c);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(u"\ntwo"_s);

        VERIFY_ALL_HOLD(f, u"one\ntwo"_s);
        QCOMPARE(textOf_(c), u"one\ntwo"_s);
    }

    // --- Undo and redo ------------------------------------------------------

    void undoWithNothingToUndoChangesNothing()
    {
        Fixture_ f(u"one"_s);

        f.prime.undo();
        f.prime.redo();

        VERIFY_ALL_HOLD(f, u"one"_s);
    }

    // A view has no undo of its own to move its cursor, so the prime says
    // where the change was
    void undoAndRedoHintWhereTheChangeWas()
    {
        Fixture_ f(u"one two"_s);

        QTextCursor cursor(&f.a);
        cursor.setPosition(3);
        cursor.insertText(u"XYZ"_s);

        QSignalSpy hint(&f.prime, &PrimeDocument::cursorPositionHint);

        // Undo removes "XYZ": the cursor belongs where it was
        f.prime.undo();
        QCOMPARE(hint.count(), 1);
        QCOMPARE(hint.takeFirst().first().toInt(), 3);

        // Redo puts it back: the cursor belongs after it
        f.prime.redo();
        QCOMPARE(hint.count(), 1);
        QCOMPARE(hint.takeFirst().first().toInt(), 6);
    }

    // An edit block in a view reaches the prime as one edit, so it undoes as
    // one step. The typing helpers rely on this (a pair inserted, a selection
    // wrapped), as Replace All does
    void editBlockInAViewUndoesAsOneStep()
    {
        Fixture_ f(u"one two"_s);

        QTextCursor cursor(&f.a);
        cursor.beginEditBlock();
        cursor.setPosition(7);
        cursor.insertText(u")"_s);
        cursor.setPosition(4);
        cursor.insertText(u"("_s);
        cursor.endEditBlock();

        VERIFY_ALL_HOLD(f, u"one (two)"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one two"_s);

        f.prime.redo();
        VERIFY_ALL_HOLD(f, u"one (two)"_s);
    }

    // --- Changes made on the prime ------------------------------------------

    void setTextReachesEveryViewAndClearsUndo()
    {
        Fixture_ f(u"one"_s);

        QTextCursor cursor(&f.a);
        cursor.insertText(u"X"_s);
        QVERIFY(f.prime.isUndoAvailable());

        f.prime.setText(u"two\nthree"_s);

        VERIFY_ALL_HOLD(f, u"two\nthree"_s);
        QVERIFY(!f.prime.isUndoAvailable());
    }

    void replaceAllUndoableReachesEveryViewAndUndoes()
    {
        Fixture_ f(u"one\ntwo"_s);

        f.prime.replaceAllUndoable(u"three\nfour\nfive"_s);
        VERIFY_ALL_HOLD(f, u"three\nfour\nfive"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one\ntwo"_s);

        f.prime.redo();
        VERIFY_ALL_HOLD(f, u"three\nfour\nfive"_s);
    }

    void replaceAllUndoableWithEmptyText()
    {
        Fixture_ f(u"one\ntwo"_s);

        f.prime.replaceAllUndoable(u""_s);
        VERIFY_ALL_HOLD(f, u""_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one\ntwo"_s);
    }

    void replaceAllUndoableOnAnEmptyDocument()
    {
        Fixture_ f(u""_s);

        f.prime.replaceAllUndoable(u"one\ntwo"_s);
        VERIFY_ALL_HOLD(f, u"one\ntwo"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u""_s);
    }

    // After a reload, typing and undo still run through every view, and the
    // typing undoes on its own.
    //
    // The setModified(false) is what TextFileModel::reloadContent does next,
    // and the second half of this depends on it: a text document folds an
    // insertion into the one before it when the two touch and the document
    // is marked modified. Without the call, text typed at the end of the
    // reloaded text joins the reload's own undo step, and one undo takes
    // back both
    void editAfterReplaceAllUndoable()
    {
        Fixture_ f(u"one"_s);

        f.prime.replaceAllUndoable(u"two"_s);
        f.prime.setModified(false);

        QTextCursor cursor(&f.b);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(u"!"_s);
        VERIFY_ALL_HOLD(f, u"two!"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"two"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one"_s);
    }

    // insertText puts its text at the start of the document
    void insertTextReachesEveryView()
    {
        Fixture_ f(u"one"_s);

        f.prime.insertText(u"A\nB"_s);
        VERIFY_ALL_HOLD(f, u"A\nBone"_s);

        f.prime.undo();
        VERIFY_ALL_HOLD(f, u"one"_s);
    }

    // --- Views coming and going ---------------------------------------------

    void aDestroyedViewIsDropped()
    {
        Fixture_ f(u"one"_s);

        {
            QTextDocument c{};
            setUpView_(c, f.prime);
            QCOMPARE(f.prime.viewCount(), 3);
        }

        QCOMPARE(f.prime.viewCount(), 2);

        QTextCursor cursor(&f.a);
        cursor.insertText(u"X"_s);

        VERIFY_ALL_HOLD(f, u"Xone"_s);
    }

    // The content outlives every view
    void thePrimeKeepsItsTextWithNoViews()
    {
        PrimeDocument prime(nullptr);
        prime.setText(u"one"_s);

        {
            QTextDocument view{};
            setUpView_(view, prime);

            QTextCursor cursor(&view);
            cursor.insertText(u"X"_s);
        }

        QCOMPARE(prime.viewCount(), 0);
        QCOMPARE(prime.text(), u"Xone"_s);

        prime.undo();
        QCOMPARE(prime.text(), u"one"_s);
    }

    void anUnregisteredViewNoLongerTakesPart()
    {
        Fixture_ f(u"one"_s);

        f.prime.unregisterView(&f.b);
        QCOMPARE(f.prime.viewCount(), 1);

        // An edit elsewhere doesn't reach it
        QTextCursor in_a(&f.a);
        in_a.insertText(u"X"_s);

        QCOMPARE(f.prime.text(), u"Xone"_s);
        QCOMPARE(textOf_(f.a), u"Xone"_s);
        QCOMPARE(textOf_(f.b), u"one"_s);

        // And an edit made in it reaches nothing
        QTextCursor in_b(&f.b);
        in_b.insertText(u"Y"_s);

        QCOMPARE(f.prime.text(), u"Xone"_s);
        QCOMPARE(textOf_(f.a), u"Xone"_s);
    }

    // --- Drift --------------------------------------------------------------

    // A view's document is changed without the prime hearing of it, which
    // nothing in the app does. The next edit, from anywhere, finds the view
    // out of step and resets it from the prime

    void aDriftedViewIsResetByAnEditElsewhere()
    {
        Fixture_ f(u"one"_s);

        f.a.blockSignals(true);
        QTextCursor unheard(&f.a);
        unheard.insertText(u"DRIFT"_s);
        f.a.blockSignals(false);

        QCOMPARE(textOf_(f.a), u"DRIFTone"_s);
        QCOMPARE(f.prime.text(), u"one"_s);

        QTextCursor in_b(&f.b);
        in_b.movePosition(QTextCursor::End);
        in_b.insertText(u"!"_s);

        VERIFY_ALL_HOLD(f, u"one!"_s);
    }

    void aDriftedViewIsResetByItsOwnEdit()
    {
        Fixture_ f(u"one"_s);

        f.a.blockSignals(true);
        QTextCursor unheard(&f.a);
        unheard.insertText(u"DRIFT"_s);
        f.a.blockSignals(false);

        QTextCursor in_a(&f.a);
        in_a.movePosition(QTextCursor::End);
        in_a.insertText(u"!"_s);

        // The edit was made past the end of the prime's text, so it lands at
        // the end
        VERIFY_ALL_HOLD(f, u"one!"_s);

        // The views are in step again, so the next edit goes where it is made
        QTextCursor again(&f.a);
        again.insertText(u"X"_s);

        VERIFY_ALL_HOLD(f, u"Xone!"_s);
    }

    void aDriftedViewIsResetByUndo()
    {
        Fixture_ f(u"one"_s);

        QTextCursor in_b(&f.b);
        in_b.insertText(u"X"_s);

        f.a.blockSignals(true);
        QTextCursor unheard(&f.a);
        unheard.insertText(u"DRIFT"_s);
        f.a.blockSignals(false);

        f.prime.undo();

        VERIFY_ALL_HOLD(f, u"one"_s);
    }

    // --- Signals ------------------------------------------------------------

    void signalsFollowAnEditAndItsUndo()
    {
        Fixture_ f(u"one"_s);
        f.prime.setModified(false);

        QSignalSpy contents(&f.prime, &PrimeDocument::contentsChange);
        QSignalSpy modification(&f.prime, &PrimeDocument::modificationChanged);
        QSignalSpy undo_available(&f.prime, &PrimeDocument::undoAvailable);
        QSignalSpy redo_available(&f.prime, &PrimeDocument::redoAvailable);

        QTextCursor cursor(&f.a);
        cursor.insertText(u"X"_s);

        // One edit in a view is one change on the prime, however many views
        // it then reaches
        QCOMPARE(contents.count(), 1);
        QCOMPARE(modification.count(), 1);
        QCOMPARE(modification.takeFirst().first().toBool(), true);
        QCOMPARE(undo_available.count(), 1);
        QCOMPARE(undo_available.takeFirst().first().toBool(), true);

        f.prime.undo();

        QCOMPARE(contents.count(), 2);
        QCOMPARE(modification.count(), 1);
        QCOMPARE(modification.takeFirst().first().toBool(), false);
        QCOMPARE(undo_available.count(), 1);
        QCOMPARE(undo_available.takeFirst().first().toBool(), false);
        QVERIFY(redo_available.count() >= 1);
        QCOMPARE(redo_available.last().first().toBool(), true);
    }
};

#undef VERIFY_ALL_HOLD

QTEST_MAIN(PrimeDocumentTest)

#include "PrimeDocumentTest.moc"
