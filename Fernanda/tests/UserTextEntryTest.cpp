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

#include <QObject>
#include <QString>
#include <QTest>

#include "ui/widgets/UserTextEntry.h"

using namespace Qt::StringLiterals;

using Problem = Fernanda::Ui::UserTextEntry::Problem;

Q_DECLARE_METATYPE(Problem)

// Which names a new or renamed file or folder may take (ui/widgets/
// UserTextEntry.h). Text outside ASCII is written as escapes, so the cases
// don't depend on how the compiler reads this file
class UserTextEntryTest : public QObject
{
    Q_OBJECT

private slots:
    void problemOf_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<Problem>("expected");

        QTest::newRow("plain") << u"Chapter 1"_s << Problem::None;
        QTest::newRow("extension") << u"notes.txt"_s << Problem::None;
        QTest::newRow("inner dots") << u"v1.2 draft.md"_s << Problem::None;
        QTest::newRow("accented") << u"café"_s << Problem::None;
        QTest::newRow("empty") << u""_s << Problem::Empty;

        // Forbidden characters
        QTest::newRow("slash") << u"a/b"_s << Problem::ForbiddenChar;
        QTest::newRow("backslash") << u"a\\b"_s << Problem::ForbiddenChar;
        QTest::newRow("colon") << u"a:b"_s << Problem::ForbiddenChar;
        QTest::newRow("question") << u"why?"_s << Problem::ForbiddenChar;
        QTest::newRow("tab") << u"a\tb"_s << Problem::ForbiddenChar;

        // Leading and trailing
        QTest::newRow("leading dot") << u".hidden"_s << Problem::LeadingDot;
        QTest::newRow("trailing dot")
            << u"end."_s << Problem::TrailingDotOrSpace;
        QTest::newRow("trailing space")
            << u"end "_s << Problem::TrailingDotOrSpace;

        // Reserved device names, with or without an extension, in any case
        QTest::newRow("CON") << u"CON"_s << Problem::ReservedName;
        QTest::newRow("con.txt") << u"con.txt"_s << Problem::ReservedName;
        QTest::newRow("NUL.tar.gz") << u"NUL.tar.gz"_s << Problem::ReservedName;
        QTest::newRow("COM1") << u"COM1"_s << Problem::ReservedName;
        QTest::newRow("lpt9.md") << u"lpt9.md"_s << Problem::ReservedName;

        // Superscript one, two, and three count as digits in a port name
        QTest::newRow("COM superscript one")
            << u"COM¹"_s << Problem::ReservedName;
        QTest::newRow("com superscript two.txt")
            << u"com².txt"_s << Problem::ReservedName;
        QTest::newRow("LPT superscript three")
            << u"LPT³"_s << Problem::ReservedName;

        // Close to a reserved name, but not one
        QTest::newRow("COM0") << u"COM0"_s << Problem::None;
        QTest::newRow("COM10") << u"COM10"_s << Problem::None;
        QTest::newRow("CONSOLE") << u"CONSOLE"_s << Problem::None;
        QTest::newRow("contents.txt") << u"contents.txt"_s << Problem::None;
        QTest::newRow("my con") << u"my con"_s << Problem::None;
    }

    void problemOf()
    {
        QFETCH(QString, name);
        QFETCH(Problem, expected);

        QCOMPARE(Fernanda::Ui::UserTextEntry::problemOf(name), expected);
    }
};

QTEST_GUILESS_MAIN(UserTextEntryTest)

#include "UserTextEntryTest.moc"
