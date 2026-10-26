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

#include "core/git/GitRefName.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Making a branch name acceptable to git as it's typed
// (core/git/GitRefName.h), after GitHub Desktop's sanitizedRefName.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class GitRefNameTest : public QObject
{
    Q_OBJECT

private slots:
    void sanitizes_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expected");

        QTest::newRow("a good name") << u"draft-two"_s << u"draft-two"_s;
        QTest::newRow("folders") << u"drafts/two"_s << u"drafts/two"_s;
        QTest::newRow("a space") << u"chapter two"_s << u"chapter-two"_s;
        QTest::newRow("a run of spaces") << u"a   b"_s << u"a-b"_s;
        QTest::newRow("refused characters")
            << u"a:b?c*d[e\\f~g^h"_s << u"a-b-c-d-e-f-g-h"_s;
        QTest::newRow("Windows' characters")
            << u"a|b\"c<d>e"_s << u"a-b-c-d-e"_s;
        QTest::newRow("a refused run") << u"a :?b"_s << u"a-b"_s;
        QTest::newRow("at-brace") << u"a@{b"_s << u"a-b"_s;
        QTest::newRow("an at alone") << u"a@b"_s << u"a@b"_s;
        QTest::newRow("two dots") << u"a..b"_s << u"a-b"_s;
        QTest::newRow("one dot") << u"v1.2"_s << u"v1.2"_s;
        QTest::newRow("a leading dot") << u".hidden"_s << u"hidden"_s;
        QTest::newRow("a trailing dot") << u"name."_s << u"name-"_s;
        QTest::newRow("a trailing .lock") << u"name.lock"_s << u"name-"_s;
        QTest::newRow("a trailing slash") << u"name/"_s << u"name-"_s;
        QTest::newRow("leading hyphens and pluses")
            << u"-+-name"_s << u"name"_s;
        QTest::newRow("a leading space") << u" name"_s << u"name"_s;
        QTest::newRow("a tab") << u"a\tb"_s << u"a-b"_s;
        QTest::newRow("text outside ASCII") << u"café"_s << u"café"_s;
        QTest::newRow("nothing left") << u"   "_s << QString{};
        QTest::newRow("empty") << QString{} << QString{};
    }

    void sanitizes()
    {
        QFETCH(QString, name);
        QFETCH(QString, expected);

        QCOMPARE(GitRefName::sanitized(name), expected);
    }
};

QTEST_GUILESS_MAIN(GitRefNameTest)
#include "GitRefNameTest.moc"
