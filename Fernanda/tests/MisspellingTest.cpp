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
#include <QChar>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/spell/Misspelling.h"
#include "core/spell/SpellChecker.h"
#include "core/spell/WordList.h"

using namespace Qt::StringLiterals;

using Fernanda::SpellChecker;
using Fernanda::WordList;

namespace Misspelling = Fernanda::Misspelling;

// Whether a word is misspelled, given a dictionary and a list of accepted
// words (core/spell/Misspelling.h). Each test writes a dictionary of a few
// words to a temporary folder, so nothing here depends on a real one being
// installed.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name.
// Text outside ASCII is built from code point numbers, so the cases don't
// depend on how the compiler reads this file
class MisspellingTest : public QObject
{
    Q_OBJECT

private:
    // A dictionary on disk, in a temporary folder
    struct Fixture_
    {
        Fixture_(const QByteArray& affix, const QByteArray& words)
        {
            QDir dir(folder.path());

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
    // flagged M may take a possessive 's
    [[nodiscard]] static QByteArray affix_()
    {
        return "SET UTF-8\n"
               "TRY esianrtolcdugmphbyfvkwz'\n"
               "WORDCHARS '\n"
               "SFX M Y 1\n"
               "SFX M 0 's .\n";
    }

    // The number of words, then one to a line. Only "Paris" takes the
    // possessive
    [[nodiscard]] static QByteArray words_()
    {
        return "5\n"
               "hello\n"
               "like\n"
               "well\n"
               "known\n"
               "Paris/M\n";
    }

    // One word stored with a capital and one in lowercase, which accept
    // different forms (WordList)
    [[nodiscard]] static WordList accepted_()
    {
        WordList list{};
        list.insert(u"Pangloss"_s);
        list.insert(u"zorb"_s);

        return list;
    }

    [[nodiscard]] static QString typeset_() { return QString(QChar(0x2019)); }

    [[nodiscard]] static QString nonBreakingHyphen_()
    {
        return QString(QChar(0x2011));
    }

private slots:
    void isMisspelled_data()
    {
        QTest::addColumn<QString>("word");
        QTest::addColumn<bool>("expected");

        QTest::newRow("in the dictionary") << u"hello"_s << false;
        QTest::newRow("in neither") << u"helo"_s << true;
        QTest::newRow("accepted") << u"Pangloss"_s << false;
        QTest::newRow("accepted, by its lowercase rule") << u"Zorb"_s << false;
        QTest::newRow("accepted, against its capital") << u"pangloss"_s << true;

        QTest::newRow("possessive of an accepted word")
            << u"Pangloss's"_s << false;
        QTest::newRow("possessive, typographic apostrophe")
            << u"Pangloss"_s + typeset_() + u"s"_s << false;
        QTest::newRow("possessive, all caps") << u"PANGLOSS'S"_s << false;
        QTest::newRow("possessive, by the lowercase rule")
            << u"Zorb's"_s << false;
        QTest::newRow("possessive of a misspelled word")
            << u"Panglos's"_s << true;
        QTest::newRow("possessive, against the capital")
            << u"pangloss's"_s << true;

        // The 's is removed for the accepted words only: which dictionary
        // words take one is the dictionary's to say
        QTest::newRow("possessive the dictionary allows")
            << u"Paris's"_s << false;
        QTest::newRow("possessive the dictionary doesn't")
            << u"hello's"_s << true;

        QTest::newRow("hyphenated, both in the dictionary")
            << u"well-known"_s << false;
        QTest::newRow("hyphenated, accepted then dictionary")
            << u"Pangloss-like"_s << false;
        QTest::newRow("hyphenated, dictionary then accepted")
            << u"well-zorb"_s << false;
        QTest::newRow("hyphenated, both accepted")
            << u"zorb-Pangloss"_s << false;
        QTest::newRow("hyphenated, three parts")
            << u"well-Pangloss-like"_s << false;
        QTest::newRow("hyphenated, non-breaking hyphen")
            << u"Pangloss"_s + nonBreakingHyphen_() + u"like"_s << false;
        QTest::newRow("hyphenated, possessive part")
            << u"well-Pangloss's"_s << false;
        QTest::newRow("hyphenated, one part misspelled")
            << u"Pangloss-xyzzy"_s << true;
        QTest::newRow("hyphenated, first part misspelled")
            << u"xyzzy-Pangloss"_s << true;
    }

    void isMisspelled()
    {
        QFETCH(QString, word);
        QFETCH(bool, expected);

        Fixture_ f(affix_(), words_());
        SpellChecker checker(f.affixFile, f.wordFile);
        QVERIFY(checker.isValid());

        QCOMPARE(
            Misspelling::isMisspelled(checker, accepted_(), word),
            expected);
    }

    void aHyphenatedWordAcceptedWholeIsCorrect()
    {
        Fixture_ f(affix_(), words_());
        SpellChecker checker(f.affixFile, f.wordFile);

        WordList accepted{};
        accepted.insert(u"Jean-Luc"_s);

        QVERIFY(!Misspelling::isMisspelled(checker, accepted, u"Jean-Luc"_s));
        QVERIFY(!Misspelling::isMisspelled(checker, accepted, u"Jean-Luc's"_s));
        QVERIFY(Misspelling::isMisspelled(checker, accepted, u"Jean"_s));
    }

    void nothingIsMisspelledWithoutADictionary()
    {
        QTemporaryDir folder{};
        QDir dir(folder.path());

        SpellChecker checker(
            Coco::Path(dir.filePath(u"missing.aff"_s)),
            Coco::Path(dir.filePath(u"missing.dic"_s)));

        QVERIFY(!checker.isValid());
        QVERIFY(!Misspelling::isMisspelled(checker, WordList{}, u"xyzzy"_s));
        QVERIFY(
            !Misspelling::isMisspelled(checker, WordList{}, u"xyzzy-helo"_s));
    }

    void withoutPossessive_data()
    {
        QTest::addColumn<QString>("word");
        QTest::addColumn<QString>("expected");

        QTest::newRow("none") << u"Pangloss"_s << u"Pangloss"_s;
        QTest::newRow("plain apostrophe") << u"Pangloss's"_s << u"Pangloss"_s;
        QTest::newRow("typographic apostrophe")
            << u"Pangloss"_s + typeset_() + u"s"_s << u"Pangloss"_s;
        QTest::newRow("all caps") << u"PANGLOSS'S"_s << u"PANGLOSS"_s;
        QTest::newRow("hyphenated") << u"Jean-Luc's"_s << u"Jean-Luc"_s;
        QTest::newRow("another contraction") << u"don't"_s << u"don't"_s;
        QTest::newRow("ends in s") << u"Candides"_s << u"Candides"_s;
        QTest::newRow("nothing before it") << u"'s"_s << u"'s"_s;
    }

    void withoutPossessive()
    {
        QFETCH(QString, word);
        QFETCH(QString, expected);

        QCOMPARE(Misspelling::withoutPossessive(word), expected);
    }
};

QTEST_GUILESS_MAIN(MisspellingTest)

#include "MisspellingTest.moc"
