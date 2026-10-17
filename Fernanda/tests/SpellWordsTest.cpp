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

#include <QChar>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTest>

#include "core/spell/SpellWords.h"

using namespace Qt::StringLiterals;

namespace SpellWords = Fernanda::SpellWords;

// Which stretches of a line are words to check the spelling of
// (core/spell/SpellWords.h).
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name.
// Text outside ASCII is built from code point numbers, so the cases don't
// depend on how the compiler reads this file
class SpellWordsTest : public QObject
{
    Q_OBJECT

private:
    // The words found in a line, as text
    [[nodiscard]] static QStringList wordsIn_(const QString& line)
    {
        QStringList words{};

        for (const auto& range : SpellWords::find(line)) {
            words << line.mid(range.start, range.length);
        }

        return words;
    }

    [[nodiscard]] static QString ch_(char32_t codePoint)
    {
        return QString::fromUcs4(&codePoint, 1);
    }

private slots:
    void find_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("empty") << u""_s << QStringList{};
        QTest::newRow("spaces only") << u"   "_s << QStringList{};
        QTest::newRow("one word") << u"hello"_s << QStringList{ u"hello"_s };
        QTest::newRow("a sentence")
            << u"In a castle of Westphalia, lived a youth."_s
            << QStringList{
                   u"In"_s,         u"a"_s,     u"castle"_s, u"of"_s,
                   u"Westphalia"_s, u"lived"_s, u"a"_s,      u"youth"_s
               };
        QTest::newRow("tabs and several spaces")
            << u"one\t two   three"_s
            << QStringList{ u"one"_s, u"two"_s, u"three"_s };

        QTest::newRow("an apostrophe inside a word")
            << u"don't stop"_s << QStringList{ u"don't"_s, u"stop"_s };
        QTest::newRow("a typographic apostrophe inside a word")
            << (u"don"_s + ch_(0x2019) + u"t stop"_s)
            << QStringList{ u"don"_s + ch_(0x2019) + u"t"_s, u"stop"_s };
        QTest::newRow("a possessive")
            << u"the Baron's castle"_s
            << QStringList{ u"the"_s, u"Baron's"_s, u"castle"_s };
        QTest::newRow("quotation marks around a word")
            << u"he said 'this' twice"_s
            << QStringList{ u"he"_s, u"said"_s, u"this"_s, u"twice"_s };
        QTest::newRow("an apostrophe after the last letter")
            << u"the dogs' bowls"_s
            << QStringList{ u"the"_s, u"dogs"_s, u"bowls"_s };

        QTest::newRow("a hyphen inside a word")
            << u"a well-known man"_s
            << QStringList{ u"a"_s, u"well-known"_s, u"man"_s };
        QTest::newRow("a hyphen with spaces around it")
            << u"well - known"_s << QStringList{ u"well"_s, u"known"_s };
        QTest::newRow("two hyphens in a row")
            << u"word--word"_s << QStringList{ u"word"_s, u"word"_s };
        QTest::newRow("a hyphen at either end")
            << u"-well known-"_s << QStringList{ u"well"_s, u"known"_s };
        QTest::newRow("an em dash") << (u"word"_s + ch_(0x2014) + u"word"_s)
                                    << QStringList{ u"word"_s, u"word"_s };

        QTest::newRow("a period between letters")
            << u"e.g. this"_s << QStringList{ u"e"_s, u"g"_s, u"this"_s };
        QTest::newRow("an underscore between letters")
            << u"snake_case"_s << QStringList{ u"snake"_s, u"case"_s };
        QTest::newRow("Markdown marks")
            << u"# A **bold** _word_"_s
            << QStringList{ u"A"_s, u"bold"_s, u"word"_s };

        QTest::newRow("digits stay in their word")
            << u"the 3rd mp3 of 1984"_s
            << QStringList{ u"the"_s, u"3rd"_s, u"mp3"_s, u"of"_s, u"1984"_s };

        // "caf" and an e with an acute accent, as one character and as an e
        // followed by a combining accent
        QTest::newRow("an accented letter")
            << (u"caf"_s + ch_(0x00E9) + u" au lait"_s)
            << QStringList{ u"caf"_s + ch_(0x00E9), u"au"_s, u"lait"_s };
        QTest::newRow("a combining accent")
            << (u"cafe"_s + ch_(0x0301) + u" au"_s)
            << QStringList{ u"cafe"_s + ch_(0x0301), u"au"_s };

        // Two Han characters between two English words
        QTest::newRow("Chinese characters are not words")
            << (u"one "_s + ch_(0x6587) + ch_(0x5B57) + u" two"_s)
            << QStringList{ u"one"_s, u"two"_s };

        // An emoji, outside the BMP, between two words with no spaces
        QTest::newRow("an emoji separates")
            << (u"one"_s + ch_(0x1F4C1) + u"two"_s)
            << QStringList{ u"one"_s, u"two"_s };
    }

    void find()
    {
        QFETCH(QString, line);
        QFETCH(QStringList, expected);

        QCOMPARE(wordsIn_(line), expected);
    }

    // A word after a character outside the BMP starts two units later
    void rangesCountUtf16Units()
    {
        auto line = ch_(0x1F4C1) + u" one"_s;
        auto words = SpellWords::find(line);

        QCOMPARE(words.size(), 1);
        QCOMPARE(words.at(0).start, 3);
        QCOMPARE(words.at(0).length, 3);
    }
};

QTEST_GUILESS_MAIN(SpellWordsTest)

#include "SpellWordsTest.moc"
