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

#include <QByteArray>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <Coco/Path.h>

#include "core/PathKey.h"
#include "models/AbstractFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using Fernanda::TextFileModel;

// What a text buffer does to a file's bytes (models/TextFileModel.h): what it
// hands back unchanged, what it changes, what it accepts as UTF-8, and how an
// edit and a reload leave it.
//
// Bytes outside ASCII are written as \x escapes. Where a letter that is also
// a hex digit follows one, the literal is split in two ("\xA0" "b"), since
// "\xA0b" would read as one escape.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class TextFileModelTest : public QObject
{
    Q_OBJECT

private:
    // A view's document, set up as TextFileView sets its own up. Edits made
    // to it reach the model as a user's typing does
    class ViewDocument_
    {
    public:
        explicit ViewDocument_(TextFileModel& model)
        {
            document_.setDocumentLayout(
                new QPlainTextDocumentLayout(&document_));
            model.registerView(&document_);
        }

        void typeAtEnd(const QString& text)
        {
            QTextCursor cursor(&document_);
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(text);
        }

    private:
        QTextDocument document_{};
    };

    // No file is needed: the model only stores its key
    [[nodiscard]] static Fernanda::PathKey key_()
    {
        return Fernanda::PathKey::of(Coco::Path::Temp("a.txt"));
    }

private slots:
    // --- Bytes that come back unchanged -------------------------------------

    void roundTrip_data()
    {
        QTest::addColumn<QByteArray>("bytes");

        QTest::newRow("empty") << QByteArray("");
        QTest::newRow("one line, no break") << QByteArray("one");
        QTest::newRow("LF") << QByteArray("one\ntwo\n");
        QTest::newRow("LF, no final break") << QByteArray("one\ntwo");
        QTest::newRow("CRLF") << QByteArray("one\r\ntwo\r\n");
        QTest::newRow("CRLF, no final break") << QByteArray("one\r\ntwo");
        QTest::newRow("only LF") << QByteArray("\n");
        QTest::newRow("only CRLF") << QByteArray("\r\n");
        QTest::newRow("blank lines") << QByteArray("one\n\n\ntwo\n\n");

        QTest::newRow("BOM, LF") << QByteArray("\xEF\xBB\xBFone\n");
        QTest::newRow("BOM, CRLF") << QByteArray("\xEF\xBB\xBFone\r\n");
        QTest::newRow("BOM alone") << QByteArray("\xEF\xBB\xBF");

        // U+FEFF past the start is a character, not a mark
        QTest::newRow("U+FEFF inside") << QByteArray(
            "a\xEF\xBB\xBF"
            "b");

        QTest::newRow("tabs and trailing spaces")
            << QByteArray("a\t b  \n\t\n");
        QTest::newRow("accented") << QByteArray("caf\xC3\xA9\n");
        QTest::newRow("Japanese")
            << QByteArray("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\n");
        QTest::newRow("emoji") << QByteArray("\xF0\x9F\x93\x81\n");

        // Characters a plain-text widget's own text accessor rewrites
        QTest::newRow("no-break space") << QByteArray(
            "a\xC2\xA0"
            "b\n");
        QTest::newRow("line separator U+2028") << QByteArray(
            "a\xE2\x80\xA8"
            "b\n");

        // A replacement character already in the file is ordinary text
        QTest::newRow("U+FFFD") << QByteArray(
            "a\xEF\xBF\xBD"
            "b\n");

        QTest::newRow("soft hyphen") << QByteArray(
            "a\xC2\xAD"
            "b\n");
        QTest::newRow("zero-width joiner") << QByteArray(
            "a\xE2\x80\x8D"
            "b\n");
        QTest::newRow("next line U+0085") << QByteArray(
            "a\xC2\x85"
            "b\n");
        QTest::newRow("object replacement U+FFFC") << QByteArray(
            "a\xEF\xBF\xBC"
            "b\n");

        // Control characters
        QTest::newRow("form feed") << QByteArray("a\fb\n");
        QTest::newRow("vertical tab") << QByteArray("a\vb\n");
        QTest::newRow("escape") << QByteArray(
            "a\x1B"
            "b\n");
        QTest::newRow("delete") << QByteArray(
            "a\x7F"
            "b\n");
        QTest::newRow("NUL") << QByteArray("a\0b\n", 4);
    }

    void roundTrip()
    {
        QFETCH(QByteArray, bytes);

        TextFileModel model(key_(), nullptr);
        model.setData(bytes);

        QCOMPARE(model.data(), bytes);
        QVERIFY(!model.isModified());
    }

    // --- Bytes that come back changed ---------------------------------------

    // The cases a load can't hand back as they were. Every one is a line
    // break in a form the text document doesn't keep
    void normalized_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<QByteArray>("expected");

        // A bare CR is a break, and saves as the file's line ending
        QTest::newRow("bare CR")
            << QByteArray("one\rtwo") << QByteArray("one\ntwo");
        QTest::newRow("LF then CR")
            << QByteArray("a\n\rb") << QByteArray("a\n\nb");

        // The first LF decides the file's line ending: CRLF if a CR comes
        // before it. Every break is then saved that way
        QTest::newRow("CRLF then LF")
            << QByteArray("a\r\nb\nc") << QByteArray("a\r\nb\r\nc");
        QTest::newRow("LF then CRLF")
            << QByteArray("a\nb\r\nc") << QByteArray("a\nb\nc");
        QTest::newRow("bare CR then CRLF")
            << QByteArray("a\rb\r\nc") << QByteArray("a\r\nb\r\nc");
        QTest::newRow("CR CR LF")
            << QByteArray("a\r\r\nb") << QByteArray("a\r\n\r\nb");

        // Three characters the text document itself reads as a break: the
        // paragraph separator, and two values it reserves for its own use
        QTest::newRow("paragraph separator U+2029") << QByteArray(
                                                           "a\xE2\x80\xA9"
                                                           "b\n")
                                                    << QByteArray("a\nb\n");
        QTest::newRow("U+FDD0") << QByteArray(
                                       "a\xEF\xB7\x90"
                                       "b\n")
                                << QByteArray("a\nb\n");
        QTest::newRow("U+FDD1") << QByteArray(
                                       "a\xEF\xB7\x91"
                                       "b\n")
                                << QByteArray("a\nb\n");
    }

    void normalized()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(QByteArray, expected);

        TextFileModel model(key_(), nullptr);
        model.setData(bytes);

        QCOMPARE(model.data(), expected);

        // The buffer still counts as matching disk, so nothing is written
        // until the user edits
        QVERIFY(!model.isModified());
    }

    // --- UTF-8 validity -----------------------------------------------------

    void isValidUtf8_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<bool>("expected");

        QTest::newRow("empty") << QByteArray("") << true;
        QTest::newRow("ASCII") << QByteArray("abc\n") << true;
        QTest::newRow("two-byte") << QByteArray("caf\xC3\xA9") << true;
        QTest::newRow("three-byte") << QByteArray("\xE6\x97\xA5") << true;
        QTest::newRow("four-byte") << QByteArray("\xF0\x9F\x93\x81") << true;
        QTest::newRow("BOM") << QByteArray(
                                    "\xEF\xBB\xBF"
                                    "a")
                             << true;
        QTest::newRow("NUL") << QByteArray("a\0b", 3) << true;

        // A file may hold a replacement character and still be valid
        QTest::newRow("U+FFFD") << QByteArray("\xEF\xBF\xBD") << true;

        // Older encodings
        QTest::newRow("Latin-1 accent") << QByteArray("caf\xE9") << false;
        QTest::newRow("Windows-1252 quotes")
            << QByteArray("\x93hi\x94") << false;

        // A sequence cut off at the end of the file
        QTest::newRow("truncated two-byte") << QByteArray("a\xC3") << false;
        QTest::newRow("truncated three-byte")
            << QByteArray("a\xE6\x97") << false;
        QTest::newRow("truncated four-byte")
            << QByteArray("\xF0\x9F\x93") << false;

        // Bytes UTF-8 never allows
        QTest::newRow("lone continuation byte") << QByteArray("\x80") << false;
        QTest::newRow("0xFF") << QByteArray("\xFF") << false;
        QTest::newRow("overlong form") << QByteArray("\xC0\xAF") << false;
        QTest::newRow("encoded surrogate")
            << QByteArray("\xED\xA0\x80") << false;
        QTest::newRow("past U+10FFFF")
            << QByteArray("\xF4\x90\x80\x80") << false;
    }

    void isValidUtf8()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(bool, expected);

        QCOMPARE(TextFileModel::isValidUtf8(bytes), expected);
    }

    // --- Editing ------------------------------------------------------------

    void editMarksModifiedAndAnnounces()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one\n");

        QSignalSpy content_changed(
            &model,
            &Fernanda::AbstractFileModel::contentChanged);
        QSignalSpy modification_changed(
            &model,
            &Fernanda::AbstractFileModel::modificationChanged);

        ViewDocument_ view(model);
        QCOMPARE(content_changed.count(), 0);
        QVERIFY(!model.isModified());

        view.typeAtEnd(u"two"_s);

        QCOMPARE(model.data(), QByteArray("one\ntwo"));
        QVERIFY(model.isModified());
        QCOMPARE(content_changed.count(), 1);
        QCOMPARE(modification_changed.count(), 1);
        QCOMPARE(modification_changed.first().first().toBool(), true);
    }

    // A break typed into a file is saved in that file's line ending
    void editKeepsLineEnding_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("LF")
            << QByteArray("one\n") << QByteArray("one\ntwo\nthree");
        QTest::newRow("CRLF")
            << QByteArray("one\r\n") << QByteArray("one\r\ntwo\r\nthree");
        QTest::newRow("BOM, CRLF")
            << QByteArray("\xEF\xBB\xBFone\r\n")
            << QByteArray("\xEF\xBB\xBFone\r\ntwo\r\nthree");

        // A file with no break yet is LF
        QTest::newRow("no break yet")
            << QByteArray("one") << QByteArray("onetwo\nthree");
        QTest::newRow("empty") << QByteArray("") << QByteArray("two\nthree");
    }

    void editKeepsLineEnding()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(QByteArray, expected);

        TextFileModel model(key_(), nullptr);
        model.setData(bytes);

        ViewDocument_ view(model);
        view.typeAtEnd(u"two\nthree"_s);

        QCOMPARE(model.data(), expected);
    }

    void undoAndRedoAnEdit()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one\n");
        QVERIFY(!model.isUndoAvailable());

        ViewDocument_ view(model);
        view.typeAtEnd(u"two"_s);
        QVERIFY(model.isUndoAvailable());

        model.undo();
        QCOMPARE(model.data(), QByteArray("one\n"));
        QVERIFY(!model.isModified());
        QVERIFY(model.isRedoAvailable());

        model.redo();
        QCOMPARE(model.data(), QByteArray("one\ntwo"));
        QVERIFY(model.isModified());
    }

    // --- Loading and reloading ----------------------------------------------

    // setData is the open path: nothing from before it can be undone
    void setDataClearsUndo()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one\n");

        ViewDocument_ view(model);
        view.typeAtEnd(u"two"_s);
        QVERIFY(model.isUndoAvailable());

        model.setData("three\n");

        QCOMPARE(model.data(), QByteArray("three\n"));
        QVERIFY(!model.isUndoAvailable());
        QVERIFY(!model.isModified());
    }

    // reloadContent is the external-change path: the buffer matches disk
    // again, and undo brings back what it held before
    void reloadIsUndoable()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one\n");

        model.reloadContent("two\n");

        QCOMPARE(model.data(), QByteArray("two\n"));
        QVERIFY(!model.isModified());
        QVERIFY(model.isUndoAvailable());

        model.undo();

        // Now different from disk, so the next save writes it
        QCOMPARE(model.data(), QByteArray("one\n"));
        QVERIFY(model.isModified());

        model.redo();

        QCOMPARE(model.data(), QByteArray("two\n"));
    }

    // A reload over unsaved typing loses nothing: undo restores the typing
    void reloadKeepsUnsavedEditsOnTheUndoStack()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one\n");

        ViewDocument_ view(model);
        view.typeAtEnd(u"unsaved"_s);

        model.reloadContent("two\n");
        QCOMPARE(model.data(), QByteArray("two\n"));

        model.undo();
        QCOMPARE(model.data(), QByteArray("one\nunsaved"));
    }

    // Typing after a reload undoes on its own, leaving the reload in place,
    // even at the end of the reloaded text, where the typing touches it. One
    // undo taking back both would put the old text back, and the next save
    // would write it over the external change
    void typingAfterAReloadUndoesOnItsOwn()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("one");

        ViewDocument_ view(model);
        model.reloadContent("two");

        view.typeAtEnd(u"!"_s);
        QCOMPARE(model.data(), QByteArray("two!"));

        model.undo();
        QCOMPARE(model.data(), QByteArray("two"));
        QVERIFY(model.isUndoAvailable());

        model.undo();
        QCOMPARE(model.data(), QByteArray("one"));
    }

    // A reload adopts the new bytes' line ending and BOM. Undo restores the
    // text only, so the restored text is saved in the adopted format
    void reloadAdoptsFormat()
    {
        TextFileModel model(key_(), nullptr);
        model.setData("a\nb\n");

        model.reloadContent(
            "\xEF\xBB\xBF"
            "c\r\nd\r\n");
        QCOMPARE(
            model.data(),
            QByteArray(
                "\xEF\xBB\xBF"
                "c\r\nd\r\n"));

        model.undo();
        QCOMPARE(
            model.data(),
            QByteArray(
                "\xEF\xBB\xBF"
                "a\r\nb\r\n"));
    }
};

QTEST_MAIN(TextFileModelTest)

#include "TextFileModelTest.moc"
