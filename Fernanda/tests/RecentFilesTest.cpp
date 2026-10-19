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

#include <QList>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/PathKey.h"
#include "core/RecentFiles.h"

using namespace Qt::StringLiterals;

using Fernanda::PathKey;
using Fernanda::RecentFiles;

// The files a window has made active most recently (core/RecentFiles.h): the
// order they're kept in, the limit, and a list restored from a save. Two
// temporary folders stand for a project vault and the Common Vault.
//
// Qt Test runs every private slot as a test
class RecentFilesTest : public QObject
{
    Q_OBJECT

private:
    struct Fixture_
    {
        QTemporaryDir projectFolder{};
        QTemporaryDir commonFolder{};

        [[nodiscard]] PathKey inProject(const QString& relative) const
        {
            return PathKey::of(Coco::Path(projectFolder.filePath(relative)));
        }

        [[nodiscard]] PathKey inCommon(const QString& relative) const
        {
            return PathKey::of(Coco::Path(commonFolder.filePath(relative)));
        }
    };

private slots:
    void theNewestComesFirst()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject(u"a.txt"_s));
        recent.record(f.inProject(u"b.txt"_s));
        recent.record(f.inCommon(u"c.txt"_s));

        QCOMPARE(
            recent.files(),
            (QList<PathKey>{ f.inCommon(u"c.txt"_s),
                             f.inProject(u"b.txt"_s),
                             f.inProject(u"a.txt"_s) }));
    }

    // Recorded again, a file moves to the front rather than appearing twice
    void aRepeatMovesToTheFront()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject(u"a.txt"_s));
        recent.record(f.inProject(u"b.txt"_s));
        recent.record(f.inProject(u"a.txt"_s));

        QCOMPARE(
            recent.files(),
            (QList<PathKey>{ f.inProject(u"a.txt"_s),
                             f.inProject(u"b.txt"_s) }));
    }

    // The same name in two folders is two files
    void theFolderIsPartOfTheFile()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject(u"a.txt"_s));
        recent.record(f.inCommon(u"a.txt"_s));

        QCOMPARE(recent.files().size(), 2);
    }

    void theOldestFallsOffPastTheLimit()
    {
        Fixture_ f{};
        RecentFiles recent{};

        for (auto i = 0; i <= RecentFiles::MAX; ++i) {
            recent.record(f.inProject(QString::number(i)));
        }

        QCOMPARE(recent.files().size(), RecentFiles::MAX);
        QCOMPARE(
            recent.files().first(),
            f.inProject(QString::number(RecentFiles::MAX)));
        QCOMPARE(recent.files().last(), f.inProject(u"1"_s));
    }

    void anEmptyKeyIsNotRecorded()
    {
        RecentFiles recent{};

        recent.record(PathKey{});
        recent.record(PathKey::of(Coco::Path(u"relative.txt"_s)));

        QVERIFY(recent.files().isEmpty());
    }

    // A restored list keeps its order. A repeat keeps its first place, and
    // only the newest MAX are kept
    void setFilesKeepsTheSavedOrder()
    {
        Fixture_ f{};
        RecentFiles recent{};
        recent.record(f.inProject(u"old.txt"_s));

        recent.setFiles(
            { f.inProject(u"a.txt"_s),
              f.inCommon(u"b.txt"_s),
              f.inProject(u"a.txt"_s),
              f.inProject(u"c.txt"_s) });

        QCOMPARE(
            recent.files(),
            (QList<PathKey>{ f.inProject(u"a.txt"_s),
                             f.inCommon(u"b.txt"_s),
                             f.inProject(u"c.txt"_s) }));

        QList<PathKey> many{};
        for (auto i = 0; i < RecentFiles::MAX + 3; ++i) {
            many << f.inProject(QString::number(i));
        }

        recent.setFiles(many);

        QCOMPARE(recent.files().size(), RecentFiles::MAX);
        QCOMPARE(recent.files().first(), f.inProject(u"0"_s));
        QCOMPARE(
            recent.files().last(),
            f.inProject(QString::number(RecentFiles::MAX - 1)));
    }
};

QTEST_GUILESS_MAIN(RecentFilesTest)

#include "RecentFilesTest.moc"
