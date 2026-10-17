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

#include <QByteArray>
#include <QChar>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/spell/WordList.h"

using namespace Qt::StringLiterals;

using Fernanda::WordList;

// A set of words to take as correctly spelled (core/spell/WordList.h): which
// forms of a stored word it accepts, and its file.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name.
// Text outside ASCII is built from code point numbers, so the cases don't
// depend on how the compiler reads this file
class WordListTest : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] static WordList listOf_(const QStringList& words)
    {
        WordList list{};

        for (const auto& word : words) {
            list.insert(word);
        }

        return list;
    }

    [[nodiscard]] static QString ch_(char32_t codePoint)
    {
        return QString::fromUcs4(&codePoint, 1);
    }

private slots:
    void contains_data()
    {
        QTest::addColumn<QString>("stored");
        QTest::addColumn<QString>("word");
        QTest::addColumn<bool>("expected");

        QTest::newRow("lowercase, as stored")
            << u"pangloss"_s << u"pangloss"_s << true;
        QTest::newRow("lowercase, capitalized")
            << u"pangloss"_s << u"Pangloss"_s << true;
        QTest::newRow("lowercase, all caps")
            << u"pangloss"_s << u"PANGLOSS"_s << true;
        QTest::newRow("lowercase, mixed case")
            << u"pangloss"_s << u"panGloss"_s << false;

        QTest::newRow("capitalized, as stored")
            << u"Pangloss"_s << u"Pangloss"_s << true;
        QTest::newRow("capitalized, lowercase")
            << u"Pangloss"_s << u"pangloss"_s << false;
        QTest::newRow("capitalized, all caps")
            << u"Pangloss"_s << u"PANGLOSS"_s << true;

        QTest::newRow("mixed case, as stored")
            << u"iPhone"_s << u"iPhone"_s << true;
        QTest::newRow("mixed case, capitalized")
            << u"iPhone"_s << u"IPhone"_s << false;
        QTest::newRow("mixed case, all caps")
            << u"iPhone"_s << u"IPHONE"_s << true;

        QTest::newRow("all caps, lowercase") << u"NATO"_s << u"nato"_s << false;
        QTest::newRow("all caps, capitalized")
            << u"NATO"_s << u"Nato"_s << false;

        QTest::newRow("another word") << u"pangloss"_s << u"panglos"_s << false;
        QTest::newRow("empty") << u"pangloss"_s << u""_s << false;

        QTest::newRow("typographic apostrophe, stored plain")
            << u"ain't"_s << (u"ain"_s + ch_(0x2019) + u"t"_s) << true;
        QTest::newRow("plain apostrophe, stored typographic")
            << (u"ain"_s + ch_(0x2019) + u"t"_s) << u"ain't"_s << true;

        QTest::newRow("accented, capitalized")
            << (u"caf"_s + ch_(0xE9)) << (u"Caf"_s + ch_(0xE9)) << true;
        QTest::newRow("accented, all caps")
            << (u"caf"_s + ch_(0xE9)) << (u"CAF"_s + ch_(0xC9)) << true;
    }

    void contains()
    {
        QFETCH(QString, stored);
        QFETCH(QString, word);
        QFETCH(bool, expected);

        QCOMPARE(listOf_({ stored }).contains(word), expected);
    }

    // Space around a word is dropped; an empty word and a repeat aren't added
    void insertReportsWhatItAdded()
    {
        WordList list{};

        QVERIFY(list.insert(u"  Pangloss \r"_s));
        QVERIFY(list.contains(u"Pangloss"_s));

        QVERIFY(!list.insert(u"Pangloss"_s));
        QVERIFY(!list.insert(u""_s));
        QVERIFY(!list.insert(u"   "_s));

        QCOMPARE(list.size(), 1);
    }

    // Two lists together accept each one's words, each by its own rule
    void uniteAcceptsBoth()
    {
        auto list = listOf_({ u"pangloss"_s });
        list.unite(listOf_({ u"Cunegonde"_s }));

        QVERIFY(list.contains(u"Pangloss"_s));
        QVERIFY(list.contains(u"CUNEGONDE"_s));
        QVERIFY(!list.contains(u"cunegonde"_s));
        QCOMPARE(list.size(), 2);
    }

    void wordsAreSorted()
    {
        auto list = listOf_({ u"pangloss"_s, u"Cunegonde"_s, u"candide"_s });

        QCOMPARE(
            list.words(),
            (QStringList{ u"Cunegonde"_s, u"candide"_s, u"pangloss"_s }));
    }

    // A file edited by hand, with Windows line ends and blank lines, reads
    // as its words
    void readTakesAnUntidyFile()
    {
        QTemporaryDir folder{};
        auto path = folder.filePath(u"dictionary.txt"_s);

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Pangloss\r\n\r\n  candide  \n\nCunegonde");
        file.close();

        auto list = WordList::read(Coco::Path(path));

        QVERIFY(list.has_value());
        QCOMPARE(
            list->words(),
            (QStringList{ u"Cunegonde"_s, u"Pangloss"_s, u"candide"_s }));
    }

    void aMissingFileReadsAsEmpty()
    {
        QTemporaryDir folder{};
        auto list = WordList::read(Coco::Path(folder.filePath(u"none.txt"_s)));

        QVERIFY(list.has_value());
        QVERIFY(list->isEmpty());
    }

    // Written as UTF-8, sorted, one word per line; read back the same
    void writeThenReadRoundTrips()
    {
        QTemporaryDir folder{};
        Coco::Path path(folder.filePath(u"dictionary.txt"_s));

        auto cafe = u"caf"_s + ch_(0xE9);
        auto list = listOf_({ u"pangloss"_s, cafe, u"Cunegonde"_s });
        QVERIFY(list.write(path));

        QFile file(path.toQString());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(
            file.readAll(),
            QByteArray("Cunegonde\ncaf\xC3\xA9\npangloss\n"));

        auto read = WordList::read(path);
        QVERIFY(read.has_value());
        QVERIFY(*read == list);
    }
};

QTEST_GUILESS_MAIN(WordListTest)

#include "WordListTest.moc"
