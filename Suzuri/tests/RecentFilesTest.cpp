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

#include <QList>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/FileRef.h"
#include "core/RecentFiles.h"
#include "core/Vault.h"

using Suzuri::FileRef;
using Suzuri::RecentFiles;
using Suzuri::Vault;

// The files a window has made active most recently (core/RecentFiles.h): the
// order they're kept in, the limit, and a list restored from a save. Two vaults
// in temporary folders stand for a project vault and the Common Vault.
//
// Qt Test runs every private slot as a test
class RecentFilesTest : public QObject
{
    Q_OBJECT

private:
    // The vaults are declared after their folders, so destroyed first
    struct Fixture_
    {
        QTemporaryDir projectFolder{};
        QTemporaryDir commonFolder{};
        Vault project{ Coco::Path(projectFolder.path()), nullptr };
        Vault common{ Coco::Path(commonFolder.path()), nullptr };

        [[nodiscard]] FileRef inProject(const char* relative)
        {
            return { &project, Coco::Path(relative) };
        }

        [[nodiscard]] FileRef inCommon(const char* relative)
        {
            return { &common, Coco::Path(relative) };
        }
    };

private slots:
    void theNewestComesFirst()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject("a.txt"));
        recent.record(f.inProject("b.txt"));
        recent.record(f.inCommon("c.txt"));

        QCOMPARE(
            recent.files(),
            (QList<FileRef>{ f.inCommon("c.txt"),
                             f.inProject("b.txt"),
                             f.inProject("a.txt") }));
    }

    // Recorded again, a file moves to the front rather than appearing twice
    void aRepeatMovesToTheFront()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject("a.txt"));
        recent.record(f.inProject("b.txt"));
        recent.record(f.inProject("a.txt"));

        QCOMPARE(
            recent.files(),
            (QList<FileRef>{ f.inProject("a.txt"), f.inProject("b.txt") }));
    }

    // The same path in two vaults is two files
    void theVaultIsPartOfTheFile()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(f.inProject("a.txt"));
        recent.record(f.inCommon("a.txt"));

        QCOMPARE(recent.files().size(), 2);
    }

    void theOldestFallsOffPastTheLimit()
    {
        Fixture_ f{};
        RecentFiles recent{};

        for (auto i = 0; i <= RecentFiles::MAX; ++i) {
            recent.record({ &f.project, Coco::Path(QString::number(i)) });
        }

        QCOMPARE(recent.files().size(), RecentFiles::MAX);
        QCOMPARE(
            recent.files().first().relative,
            Coco::Path(QString::number(RecentFiles::MAX)));
        QCOMPARE(recent.files().last().relative, Coco::Path("1"));
    }

    void anEmptyRefIsNotRecorded()
    {
        Fixture_ f{};
        RecentFiles recent{};

        recent.record(FileRef{});
        recent.record({ &f.project, Coco::Path() });

        QVERIFY(recent.files().isEmpty());
    }

    // A restored list keeps its order. A repeat keeps its first place, and
    // only the newest MAX are kept
    void setFilesKeepsTheSavedOrder()
    {
        Fixture_ f{};
        RecentFiles recent{};
        recent.record(f.inProject("old.txt"));

        recent.setFiles(
            { f.inProject("a.txt"),
              f.inCommon("b.txt"),
              f.inProject("a.txt"),
              f.inProject("c.txt") });

        QCOMPARE(
            recent.files(),
            (QList<FileRef>{ f.inProject("a.txt"),
                             f.inCommon("b.txt"),
                             f.inProject("c.txt") }));

        QList<FileRef> many{};
        for (auto i = 0; i < RecentFiles::MAX + 3; ++i) {
            many << FileRef{ &f.project, Coco::Path(QString::number(i)) };
        }

        recent.setFiles(many);

        QCOMPARE(recent.files().size(), RecentFiles::MAX);
        QCOMPARE(recent.files().first().relative, Coco::Path("0"));
        QCOMPARE(
            recent.files().last().relative,
            Coco::Path(QString::number(RecentFiles::MAX - 1)));
    }
};

QTEST_GUILESS_MAIN(RecentFilesTest)

#include "RecentFilesTest.moc"
