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
#include <QByteArrayList>
#include <QObject>
#include <QString>
#include <QTest>

#include "core/git/GitStatus.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

Q_DECLARE_METATYPE(Fernanda::GitStatus::Change)

// Reading what `git status --porcelain=v2 --branch -z` prints
// (core/git/GitStatus.h). Output is built here record by record, in Git's
// format, with made-up modes and hashes, which the reading skips.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class GitStatusTest : public QObject
{
    Q_OBJECT

private:
    // Each record ends in a NUL, as with -z
    [[nodiscard]] static QByteArray output_(const QByteArrayList& records)
    {
        QByteArray output{};

        for (const auto& record : records) {
            output += record;
            output += '\0';
        }

        return output;
    }

    [[nodiscard]] static QByteArray
    ordinary_(const QByteArray& letters, const QByteArray& path)
    {
        return "1 " + letters + " N... 100644 100644 100644 aaaa bbbb " + path;
    }

private slots:
    void emptyOutputHasNoEntries()
    {
        auto status = GitStatus::parse({});

        QVERIFY(status.entries.isEmpty());
        QVERIFY(status.branch.name.isEmpty());
    }

    // --- Changes ------------------------------------------------------------

    void ordinaryRecordsTakeOneChange_data()
    {
        QTest::addColumn<QByteArray>("letters");
        QTest::addColumn<GitStatus::Change>("expected");

        QTest::newRow("modified in the working tree")
            << QByteArray(".M") << GitStatus::Change::Modified;
        QTest::newRow("modified in the index")
            << QByteArray("M.") << GitStatus::Change::Modified;
        QTest::newRow("modified in both")
            << QByteArray("MM") << GitStatus::Change::Modified;
        QTest::newRow("type changed")
            << QByteArray(".T") << GitStatus::Change::Modified;
        QTest::newRow("added") << QByteArray("A.") << GitStatus::Change::Added;
        QTest::newRow("added, then modified")
            << QByteArray("AM") << GitStatus::Change::Added;
        QTest::newRow("added, then deleted")
            << QByteArray("AD") << GitStatus::Change::Added;
        QTest::newRow("marked to be added (add -N)")
            << QByteArray(".A") << GitStatus::Change::Added;
        QTest::newRow("deleted in the working tree")
            << QByteArray(".D") << GitStatus::Change::Deleted;
        QTest::newRow("deleted in the index")
            << QByteArray("D.") << GitStatus::Change::Deleted;
        QTest::newRow("modified, then deleted")
            << QByteArray("MD") << GitStatus::Change::Deleted;
    }

    void ordinaryRecordsTakeOneChange()
    {
        QFETCH(QByteArray, letters);
        QFETCH(GitStatus::Change, expected);

        auto status =
            GitStatus::parse(output_({ ordinary_(letters, "a.txt") }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].change, expected);
        QCOMPARE(status.entries[0].path.prettyQString(), u"a.txt"_s);
        QVERIFY(status.entries[0].originalPath.isEmpty());
    }

    void untrackedFileIsUntracked()
    {
        auto status = GitStatus::parse(output_({ "? notes/new.md" }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].change, GitStatus::Change::Untracked);
        QCOMPARE(status.entries[0].path.prettyQString(), u"notes/new.md"_s);
    }

    void renameKeepsTheOriginalPath()
    {
        auto status = GitStatus::parse(output_(
            {
                "2 R. N... 100644 100644 100644 aaaa bbbb R100 new name.txt",
                "old name.txt",
            }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].change, GitStatus::Change::Renamed);
        QCOMPARE(status.entries[0].path.prettyQString(), u"new name.txt"_s);
        QCOMPARE(
            status.entries[0].originalPath.prettyQString(),
            u"old name.txt"_s);
    }

    void copyIsAnAddedFile()
    {
        auto status = GitStatus::parse(output_(
            {
                "2 C. N... 100644 100644 100644 aaaa bbbb C75 copy.txt",
                "a.txt",
            }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].change, GitStatus::Change::Added);
        QCOMPARE(status.entries[0].path.prettyQString(), u"copy.txt"_s);
        QVERIFY(status.entries[0].originalPath.isEmpty());
    }

    // The original path is its own record, so the one after it is read as
    // usual
    void recordAfterARenameIsRead()
    {
        auto status = GitStatus::parse(output_(
            {
                "2 R. N... 100644 100644 100644 aaaa bbbb R100 b.txt",
                "a.txt",
                "? c.txt",
            }));

        QCOMPARE(status.entries.size(), 2);
        QCOMPARE(status.entries[1].change, GitStatus::Change::Untracked);
        QCOMPARE(status.entries[1].path.prettyQString(), u"c.txt"_s);
    }

    void unmergedFileIsConflicted()
    {
        auto status = GitStatus::parse(output_(
            {
                "u UU N... 100644 100644 100644 100644 aaaa bbbb cccc c.txt",
            }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].change, GitStatus::Change::Conflicted);
        QCOMPARE(status.entries[0].path.prettyQString(), u"c.txt"_s);
    }

    // --- Paths --------------------------------------------------------------

    // Only the fields before the path are split on spaces
    void pathKeepsItsSpaces()
    {
        auto status = GitStatus::parse(
            output_({ ordinary_(".M", "Chapter One/a  b.txt") }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(
            status.entries[0].path.prettyQString(),
            u"Chapter One/a  b.txt"_s);
    }

    // "Caf\xC3\xA9.txt" is UTF-8 for a name with an e-acute
    void pathIsReadAsUtf8()
    {
        auto status =
            GitStatus::parse(output_({ ordinary_(".M", "Caf\xC3\xA9.txt") }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].path.prettyQString(), u"Caf\u00E9.txt"_s);
    }

    void malformedRecordIsSkipped()
    {
        auto status = GitStatus::parse(output_(
            {
                "1 .M too short",
                "1",
                "?",
                "? good.txt",
            }));

        QCOMPARE(status.entries.size(), 1);
        QCOMPARE(status.entries[0].path.prettyQString(), u"good.txt"_s);
    }

    // --- Branch -------------------------------------------------------------

    void branchWithAnUpstream()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.oid 0123456789abcdef0123456789abcdef01234567",
                "# branch.head main",
                "# branch.upstream origin/main",
                "# branch.ab +3 -1",
            }));

        QCOMPARE(status.branch.name, u"main"_s);
        QVERIFY(!status.branch.detached);
        QVERIFY(!status.branch.unborn);
        QCOMPARE(status.branch.upstream, u"origin/main"_s);
        QVERIFY(status.branch.upstreamFound);
        QCOMPARE(status.branch.ahead, 3);
        QCOMPARE(status.branch.behind, 1);
        QVERIFY(status.entries.isEmpty());
    }

    void branchWithoutAnUpstream()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.oid 0123456789abcdef0123456789abcdef01234567",
                "# branch.head draft-two",
            }));

        QCOMPARE(status.branch.name, u"draft-two"_s);
        QVERIFY(status.branch.upstream.isEmpty());
        QVERIFY(!status.branch.upstreamFound);
        QCOMPARE(status.branch.ahead, 0);
        QCOMPARE(status.branch.behind, 0);
    }

    // Git names an upstream that's gone from the remote, but has nothing to
    // count against, so it prints no branch.ab
    void upstreamGoneFromTheRemoteIsNotFound()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.oid 0123456789abcdef0123456789abcdef01234567",
                "# branch.head draft",
                "# branch.upstream origin/draft",
            }));

        QCOMPARE(status.branch.upstream, u"origin/draft"_s);
        QVERIFY(!status.branch.upstreamFound);
    }

    void detachedHeadHasNoName()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.oid 0123456789abcdef0123456789abcdef01234567",
                "# branch.head (detached)",
            }));

        QVERIFY(status.branch.detached);
        QVERIFY(status.branch.name.isEmpty());
    }

    void newRepositoryIsUnborn()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.oid (initial)",
                "# branch.head main",
                "? a.txt",
            }));

        QVERIFY(status.branch.unborn);
        QCOMPARE(status.branch.name, u"main"_s);
        QCOMPARE(status.entries.size(), 1);
    }

    void malformedAheadBehindLeavesZeros()
    {
        auto status = GitStatus::parse(output_(
            {
                "# branch.head main",
                "# branch.upstream origin/main",
                "# branch.ab 3 1",
            }));

        QVERIFY(!status.branch.upstreamFound);
        QCOMPARE(status.branch.ahead, 0);
        QCOMPARE(status.branch.behind, 0);
    }
};

QTEST_GUILESS_MAIN(GitStatusTest)
#include "GitStatusTest.moc"
