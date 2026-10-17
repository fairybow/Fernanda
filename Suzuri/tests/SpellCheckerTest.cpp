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

#include <string>

#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/spell/SpellChecker.h"

using namespace Qt::StringLiterals;
using Suzuri::SpellChecker;

// What the spelling checker takes for a correctly spelled word
// (core/spell/SpellChecker.h). Each test writes a dictionary of a few words to
// a temporary folder, so nothing here depends on a real one being installed.
//
// Text outside ASCII is built from code point numbers, or written as the
// bytes a dictionary file holds, so nothing here depends on how the compiler
// reads this file.
//
// Qt Test runs every private slot as a test
class SpellCheckerTest : public QObject
{
    Q_OBJECT

private:
    // A dictionary on disk, in a temporary folder. subfolder is made under
    // that folder and holds the files; empty puts them in the folder itself
    struct Fixture_
    {
        Fixture_(
            const QByteArray& affix,
            const QByteArray& words,
            const QString& subfolder)
        {
            QDir dir(folder.path());

            if (!subfolder.isEmpty()) {
                dir.mkpath(subfolder);
                dir.cd(subfolder);
            }

            affixFile = Coco::Path(dir.filePath(u"test.aff"_s));
            wordFile = Coco::Path(dir.filePath(u"test.dic"_s));

            write_(affixFile, affix);
            write_(wordFile, words);
        }

        QTemporaryDir folder{};
        Coco::Path affixFile{};
        Coco::Path wordFile{};

    private:
        static void write_(const Coco::Path& path, const QByteArray& bytes)
        {
            QFile file(path.toQString());
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(bytes);
        }
    };

    // UTF-8, an apostrophe allowed inside a word, and one rule: a word
    // flagged S may take an "s"
    [[nodiscard]] static QByteArray affix_()
    {
        return "SET UTF-8\n"
               "TRY esianrtolcdugmphbyfvkwz'\n"
               "WORDCHARS '\n"
               "SFX S Y 1\n"
               "SFX S 0 s .\n";
    }

    // The number of words, then one to a line. The last is "caf" and an e
    // with an acute accent, as UTF-8's two bytes
    [[nodiscard]] static QByteArray words_()
    {
        return "7\n"
               "hello\n"
               "world/S\n"
               "don't\n"
               "well\n"
               "known\n"
               "Paris\n"
               "caf\xC3\xA9\n";
    }

    [[nodiscard]] static QString cafe_() { return u"caf"_s + QChar(0x00E9); }

    // "don't" or "dan't", with the typographic apostrophe
    [[nodiscard]] static QString typeset_(const QString& stem)
    {
        return stem + QChar(0x2019) + u"t"_s;
    }

private slots:
    void aWordInTheDictionaryIsCorrect()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isValid());
        QVERIFY(checker.isCorrect(u"hello"_s));
        QVERIFY(checker.isCorrect(u"world"_s));
    }

    void aWordNotInTheDictionaryIsNot()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(!checker.isCorrect(u"helo"_s));
        QVERIFY(!checker.isCorrect(u"xyzzy"_s));
    }

    void theSameAnswerComesBackEachTime()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(!checker.isCorrect(u"helo"_s));
        QVERIFY(!checker.isCorrect(u"helo"_s));
        QVERIFY(checker.isCorrect(u"hello"_s));
        QVERIFY(checker.isCorrect(u"hello"_s));
    }

    void theDictionarysRulesApply()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        // "world" may take an "s"; "hello" may not
        QVERIFY(checker.isCorrect(u"worlds"_s));
        QVERIFY(!checker.isCorrect(u"hellos"_s));
    }

    void aCapitalAtTheStartOfASentenceIsFine()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(u"Hello"_s));
        QVERIFY(checker.isCorrect(u"HELLO"_s));

        // A name is listed with its capital, and needs it
        QVERIFY(checker.isCorrect(u"Paris"_s));
        QVERIFY(!checker.isCorrect(u"paris"_s));
    }

    void aWordOutsideAsciiIsChecked()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(cafe_()));
        QVERIFY(!checker.isCorrect(u"cafe"_s));
    }

    void aTypographicApostropheIsReadAsAPlainOne()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(u"don't"_s));
        QVERIFY(checker.isCorrect(typeset_(u"don"_s)));
        QVERIFY(!checker.isCorrect(typeset_(u"dan"_s)));
    }

    // Hunspell's own rule, which the checker leaves to it
    void aHyphenatedWordIsCorrectIfEachPartIs()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(u"well-known"_s));
        QVERIFY(!checker.isCorrect(u"well-knwon"_s));
        QVERIFY(!checker.isCorrect(u"wel-known"_s));
    }

    void aWordWithADigitIsNotChecked()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(u"1984"_s));
        QVERIFY(checker.isCorrect(u"xyzzy2"_s));
        QVERIFY(checker.isCorrect(u"3rd"_s));
    }

    void anEmptyWordIsCorrect()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isCorrect(QString{}));
    }

    void suggestionsOfferTheWordThatWasMeant()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.suggestions(u"helo"_s, 5).contains(u"hello"_s));
        QVERIFY(checker.suggestions(u"wrold"_s, 5).contains(u"world"_s));
        QVERIFY(checker.suggestions(u"cafe"_s, 5).contains(cafe_()));
    }

    void suggestionsStopAtTheNumberAskedFor()
    {
        Fixture_ f(affix_(), words_(), QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.suggestions(u"helo"_s, 1).size() <= 1);
        QVERIFY(checker.suggestions(u"helo"_s, 0).isEmpty());
        QVERIFY(checker.suggestions(QString{}, 5).isEmpty());
    }

    // The dictionary sits in a folder whose name is outside ASCII: an e with
    // an acute accent and a CJK character. On Windows this is the case that
    // fails if the path reaches Hunspell in the wrong form
    void aDictionaryOpensFromAPathOutsideAscii()
    {
        auto subfolder = QString(QChar(0x00E9)) + QChar(0x6587);

        Fixture_ f(affix_(), words_(), subfolder);
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isValid());
        QVERIFY(checker.isCorrect(u"hello"_s));
        QVERIFY(!checker.isCorrect(u"helo"_s));
    }

    // "caf" and an e with an acute accent, as Latin-1's one byte
    void aLatin1DictionaryWorks()
    {
        Fixture_ f("SET ISO8859-1\n", "2\nhello\ncaf\xE9\n", QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(checker.isValid());
        QVERIFY(checker.isCorrect(u"hello"_s));
        QVERIFY(checker.isCorrect(cafe_()));
        QVERIFY(!checker.isCorrect(u"cafe"_s));
        QVERIFY(checker.suggestions(u"caf"_s, 5).contains(cafe_()));
    }

    void withoutItsFilesACheckerIsNotValidAndFlagsNothing()
    {
        QTemporaryDir folder{};
        auto affix = Coco::Path(folder.filePath(u"none.aff"_s));
        auto words = Coco::Path(folder.filePath(u"none.dic"_s));

        SpellChecker checker(affix, words);

        QVERIFY(!checker.isValid());
        QVERIFY(checker.isCorrect(u"xyzzy"_s));
        QVERIFY(checker.suggestions(u"xyzzy"_s, 5).isEmpty());
    }

    void withOnlyOneOfItsFilesACheckerIsNotValid()
    {
        Fixture_ f(affix_(), words_(), QString{});
        QVERIFY(QFile::remove(f.wordFile.toQString()));

        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(!checker.isValid());
        QVERIFY(checker.isCorrect(u"xyzzy"_s));
    }

    void declaredEncoding_data()
    {
        QTest::addColumn<QByteArray>("affix");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("named")
            << QByteArray("SET UTF-8\n") << QByteArray("UTF-8");
        QTest::newRow("named, another")
            << QByteArray("SET ISO8859-15\n") << QByteArray("ISO8859-15");
        QTest::newRow("after other lines")
            << QByteArray("# a comment\nTRY abc\nSET KOI8-R\n")
            << QByteArray("KOI8-R");
        QTest::newRow("after a byte order mark") << QByteArray(
                                                        "\xEF\xBB\xBF"
                                                        "SET UTF-8\n")
                                                 << QByteArray("UTF-8");
        QTest::newRow("tabs and a carriage return")
            << QByteArray("SET\t\tUTF-8\r\n") << QByteArray("UTF-8");
        QTest::newRow("not named")
            << QByteArray("TRY abc\n") << QByteArray("ISO8859-1");
        QTest::newRow(
            "a longer word that starts the same, as Hunspell reads it")
            << QByteArray("SETTING UTF-8\n") << QByteArray("UTF-8");
        QTest::newRow("an empty file")
            << QByteArray() << QByteArray("ISO8859-1");
    }

    void declaredEncoding()
    {
        QFETCH(QByteArray, affix);
        QFETCH(QByteArray, expected);

        Fixture_ f(affix, "1\nhello\n", QString{});

        QCOMPARE(
            SpellChecker::declaredEncoding(f.affixFile),
            expected.toStdString());
    }

    void aMissingAffixFileDeclaresTheDefaultEncoding()
    {
        QTemporaryDir folder{};
        auto affix = Coco::Path(folder.filePath(u"none.aff"_s));

        QCOMPARE(
            SpellChecker::declaredEncoding(affix),
            std::string("ISO8859-1"));
    }

    // The two encodings every build of Qt converts, and a name no build does
    void canConvertTheEncodingsQtHas()
    {
        QVERIFY(SpellChecker::canConvert("UTF-8"));
        QVERIFY(SpellChecker::canConvert("ISO8859-1"));
        QVERIFY(!SpellChecker::canConvert("X-NO-SUCH-ENCODING"));
    }

    void aDictionaryInAnEncodingQtLacksIsNotValid()
    {
        Fixture_ f("SET X-NO-SUCH-ENCODING\n", "1\nhello\n", QString{});
        SpellChecker checker(f.affixFile, f.wordFile);

        QVERIFY(!checker.isValid());
        QVERIFY(checker.isCorrect(u"xyzzy"_s));
    }
};

QTEST_GUILESS_MAIN(SpellCheckerTest)

#include "SpellCheckerTest.moc"
