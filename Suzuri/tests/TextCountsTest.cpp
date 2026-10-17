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

#include <QObject>
#include <QString>
#include <QTest>

#include "core/TextCounts.h"

using namespace Qt::StringLiterals;

// What counts as a word and as a character (core/TextCounts.h). Each function
// under test takes one line of text, so no case here holds a line break.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name.
// Text outside ASCII is written as escapes, so the cases don't depend on how
// the compiler reads this file
class TextCountsTest : public QObject
{
    Q_OBJECT

private slots:
    void wordCount_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("expected");

        QTest::newRow("empty") << u""_s << 0;
        QTest::newRow("spaces only") << u"   "_s << 0;
        QTest::newRow("one word") << u"hello"_s << 1;
        QTest::newRow("two words") << u"hello world"_s << 2;
        QTest::newRow("runs of spaces and tabs")
            << u"  hello \t  world  "_s << 2;

        // Joiners: inside a word only when between two of its characters
        QTest::newRow("apostrophe") << u"don't"_s << 1;
        QTest::newRow("typeset apostrophe") << u"don\u2019t"_s << 1;
        QTest::newRow("hyphen") << u"well-known"_s << 1;
        QTest::newRow("periods") << u"e.g."_s << 1;
        QTest::newRow("decimal") << u"3.14"_s << 1;
        QTest::newRow("trailing period") << u"Mr. Smith"_s << 2;
        QTest::newRow("quoted word") << u"'quoted'"_s << 1;
        QTest::newRow("two hyphens") << u"word--word"_s << 2;
        QTest::newRow("three periods") << u"wait...what"_s << 2;

        // A comma joins only between two digits
        QTest::newRow("comma in a number") << u"1,000"_s << 1;
        QTest::newRow("commas in a number") << u"1,000,000"_s << 1;
        QTest::newRow("comma between letters") << u"a,b"_s << 2;
        QTest::newRow("comma after a digit") << u"1,a"_s << 2;
        QTest::newRow("comma and space") << u"one, two"_s << 2;

        // Separators
        QTest::newRow("em dash") << u"word\u2014word"_s << 2;
        QTest::newRow("slash") << u"and/or"_s << 2;
        QTest::newRow("ellipsis character") << u"wait\u2026what"_s << 2;

        // Only letters and digits make a word
        QTest::newRow("lone hyphen") << u"-"_s << 0;
        QTest::newRow("lone em dash") << u"\u2014"_s << 0;
        QTest::newRow("Markdown heading") << u"# Heading"_s << 1;
        QTest::newRow("Markdown bold") << u"**bold**"_s << 1;
        QTest::newRow("underscore") << u"snake_case"_s << 1;
        QTest::newRow("emoji alone") << u"\U0001F600"_s << 0;
        QTest::newRow("word and emoji") << u"hi \U0001F600"_s << 1;

        // An accent stored as its own code point belongs to its letter
        QTest::newRow("combining accent") << u"cafe\u0301"_s << 1;
        QTest::newRow("precomposed accent") << u"caf\u00E9"_s << 1;

        // Chinese characters and Japanese kana are one word each. Korean
        // uses spaces
        QTest::newRow("Han") << u"\u65E5\u672C\u8A9E"_s << 3;
        QTest::newRow("Hiragana") << u"\u3072\u3089\u304C\u306A"_s << 4;
        QTest::newRow("Katakana") << u"\u30AB\u30BF\u30AB\u30CA"_s << 4;
        QTest::newRow("Korean") << u"\uD55C\uAD6D\uC5B4 \uB2E8\uC5B4"_s << 2;
        QTest::newRow("Han beside Latin") << u"abc\u65E5\u672Cdef"_s << 4;

        QTest::newRow("sentence")
            << u"It's a well-known fact, e.g. this one."_s << 7;
    }

    void wordCount()
    {
        QFETCH(QString, text);
        QFETCH(int, expected);

        QCOMPARE(Suzuri::TextCounts::wordCount(text), expected);
    }

    void characterCount_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("expected");

        QTest::newRow("empty") << u""_s << 0;
        QTest::newRow("letters") << u"abc"_s << 3;
        QTest::newRow("spaces and a tab count") << u"a b\t"_s << 4;
        QTest::newRow("precomposed accent") << u"\u00E9"_s << 1;

        // One character to a reader, two UTF-16 code units each
        QTest::newRow("combining accent") << u"e\u0301"_s << 1;
        QTest::newRow("emoji") << u"\U0001F600"_s << 1;

        QTest::newRow("Han") << u"\u65E5\u672C\u8A9E"_s << 3;
        QTest::newRow("mixed") << u"a\U0001F600e\u0301"_s << 3;
    }

    void characterCount()
    {
        QFETCH(QString, text);
        QFETCH(int, expected);

        QCOMPARE(Suzuri::TextCounts::characterCount(text), expected);
    }
};

QTEST_GUILESS_MAIN(TextCountsTest)

#include "TextCountsTest.moc"
