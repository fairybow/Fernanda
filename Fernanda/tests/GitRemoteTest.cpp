/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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
#include <QUrl>

#include "core/git/GitRemote.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// What a remote's URL says about a GitHub repository (core/git/GitRemote.h).
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class GitRemoteTest : public QObject
{
    Q_OBJECT

private slots:
    // expected is "owner/repo", or empty for a URL that isn't a github.com
    // repository
    void gitHubRepoOfReadsEachForm_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("expected");

        QTest::newRow("https") << u"https://github.com/fairybow/Fernanda"_s
                               << u"fairybow/Fernanda"_s;
        QTest::newRow("https with .git")
            << u"https://github.com/fairybow/Fernanda.git"_s
            << u"fairybow/Fernanda"_s;
        QTest::newRow("https with a user")
            << u"https://fairybow@github.com/fairybow/Fernanda.git"_s
            << u"fairybow/Fernanda"_s;
        QTest::newRow("https with a trailing slash")
            << u"https://github.com/fairybow/Fernanda/"_s
            << u"fairybow/Fernanda"_s;
        QTest::newRow("host in capitals")
            << u"https://GitHub.com/fairybow/Fernanda"_s
            << u"fairybow/Fernanda"_s;
        QTest::newRow("scp form") << u"git@github.com:fairybow/Fernanda.git"_s
                                  << u"fairybow/Fernanda"_s;
        QTest::newRow("scp form without .git")
            << u"git@github.com:fairybow/Fernanda"_s << u"fairybow/Fernanda"_s;
        QTest::newRow("ssh") << u"ssh://git@github.com/fairybow/Fernanda.git"_s
                             << u"fairybow/Fernanda"_s;
        QTest::newRow("ssh with a port")
            << u"ssh://git@github.com:22/fairybow/Fernanda.git"_s
            << u"fairybow/Fernanda"_s;
        QTest::newRow("surrounding space")
            << u"  https://github.com/fairybow/Fernanda\n"_s
            << u"fairybow/Fernanda"_s;

        QTest::newRow("another host")
            << u"https://gitlab.com/fairybow/Fernanda.git"_s << QString{};
        QTest::newRow("a GitHub Enterprise host")
            << u"https://github.example.com/fairybow/Fernanda.git"_s
            << QString{};
        QTest::newRow("scp form, another host")
            << u"git@gitlab.com:fairybow/Fernanda.git"_s << QString{};
        QTest::newRow("a local path")
            << u"/home/me/repos/Fernanda.git"_s << QString{};
        QTest::newRow("a Windows path")
            << u"C:/Users/me/repos/Fernanda.git"_s << QString{};
        QTest::newRow("a file URL")
            << u"file:///home/me/repos/Fernanda.git"_s << QString{};
        QTest::newRow("owner only")
            << u"https://github.com/fairybow"_s << QString{};
        QTest::newRow("too deep")
            << u"https://github.com/fairybow/Fernanda/tree/main"_s << QString{};
        QTest::newRow("empty") << QString{} << QString{};
    }

    void gitHubRepoOfReadsEachForm()
    {
        QFETCH(QString, url);
        QFETCH(QString, expected);

        auto repo = GitRemote::gitHubRepoOf(url);
        QCOMPARE(repo.value_or(QString{}), expected);
    }

    void compareUrlNamesTheBranch()
    {
        auto url =
            GitRemote::compareUrl(u"fairybow/Fernanda"_s, u"git-stuff"_s);

        QCOMPARE(
            url.toString(),
            u"https://github.com/fairybow/Fernanda/compare/git-stuff?expand=1"_s);
    }

    // A branch named with a slash keeps it; a character a URL can't carry is
    // encoded
    void compareUrlKeepsSlashesAndEncodesTheRest()
    {
        auto url =
            GitRemote::compareUrl(u"fairybow/Fernanda"_s, u"drafts/two #3"_s);

        QCOMPARE(
            url.toString(QUrl::FullyEncoded),
            u"https://github.com/fairybow/Fernanda/compare/drafts/two%20%233?expand=1"_s);
    }
};

QTEST_GUILESS_MAIN(GitRemoteTest)
#include "GitRemoteTest.moc"
