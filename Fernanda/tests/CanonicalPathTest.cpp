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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QtGlobal>

#include <Coco/Path.h>

#include "core/CanonicalPath.h"

using namespace Qt::StringLiterals;

using Fernanda::CanonicalPath;

// How a path becomes a key that is the same for every spelling of one place on
// disk (core/CanonicalPath.h): resolving existing and missing paths, letter
// case, containment, and re-keying under a renamed folder.
//
// Each test works in its own temporary folder
class CanonicalPathTest : public QObject
{
    Q_OBJECT

private:
    // A temporary folder, and the file work a test does in it. Paths given to
    // these functions are relative to the folder
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

        [[nodiscard]] CanonicalPath key(const QString& relative) const
        {
            return CanonicalPath::of(absolute(relative));
        }

        void makeFolder(const QString& relative) const
        {
            QVERIFY(QDir(folder.path()).mkpath(relative));
        }

        // Creates the folders above the file too
        void write(const QString& relative) const
        {
            QFileInfo info(folder.filePath(relative));
            QVERIFY(QDir().mkpath(info.path()));

            QFile file(info.filePath());
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
    };

private slots:
    void emptyAndRelativePathsGiveAnEmptyKey()
    {
        QVERIFY(CanonicalPath::of({}).isEmpty());
        QVERIFY(CanonicalPath::of(Coco::Path(u"a/b.txt"_s)).isEmpty());
        QVERIFY(CanonicalPath().isEmpty());
    }

    void dotAndDotDotGiveTheSameKeyAsThePlainPath()
    {
        Fixture_ f{};
        f.makeFolder(u"sub"_s);
        f.write(u"notes.txt"_s);

        auto plain = f.key(u"notes.txt"_s);

        QVERIFY(!plain.isEmpty());
        QVERIFY(f.key(u"sub/../notes.txt"_s) == plain);
        QVERIFY(f.key(u"./notes.txt"_s) == plain);
    }

    void trailingSeparatorGivesTheSameKey()
    {
        Fixture_ f{};
        f.makeFolder(u"folder"_s);

        QVERIFY(f.key(u"folder/"_s) == f.key(u"folder"_s));
        QVERIFY(f.key(u"missing/"_s) == f.key(u"missing"_s));
    }

    void missingFileKeyEqualsTheKeyOnceCreated()
    {
        Fixture_ f{};
        auto before = f.key(u"new.txt"_s);

        QVERIFY(!before.isEmpty());
        QCOMPARE(before.path().nameQString(), u"new.txt"_s);

        f.write(u"new.txt"_s);
        auto after = f.key(u"new.txt"_s);

        QVERIFY(before == after);
        QCOMPARE(qHash(before), qHash(after));
    }

    void missingFoldersResolveThroughTheNearestExistingOne()
    {
        Fixture_ f{};
        auto before = f.key(u"a/b/c.txt"_s);

        QCOMPARE(before.relativeTo(f.key({})), Coco::Path(u"a/b/c.txt"_s));

        f.write(u"a/b/c.txt"_s);

        QVERIFY(f.key(u"a/b/c.txt"_s) == before);
    }

    void dotDotInAMissingPathIsResolvedAsWritten()
    {
        Fixture_ f{};

        QVERIFY(f.key(u"a/b/../c.txt"_s) == f.key(u"a/c.txt"_s));
    }

    void symlinkedFolderGivesTheSameKeyAsItsTarget()
    {
#if defined(Q_OS_WIN)

        // QFile::link makes a .lnk shortcut on Windows, not a link the file
        // system follows
        QSKIP("Needs a symlink");

#else

        Fixture_ f{};
        f.write(u"real/notes.txt"_s);
        QVERIFY(
            QFile::link(
                f.absolute(u"real"_s).toQString(),
                f.absolute(u"link"_s).toQString()));

        QVERIFY(f.key(u"link/notes.txt"_s) == f.key(u"real/notes.txt"_s));
        QVERIFY(f.key(u"link/new.txt"_s) == f.key(u"real/new.txt"_s));

#endif
    }

    void letterCaseCountsOnlyWhereTheFileSystemsDo()
    {
        Fixture_ f{};
        auto lower = f.key(u"notes.txt"_s);
        auto upper = f.key(u"NOTES.txt"_s);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        QVERIFY(lower == upper);
        QCOMPARE(qHash(lower), qHash(upper));

#else

        QVERIFY(!(lower == upper));

#endif

        QCOMPARE(upper.path().nameQString(), u"NOTES.txt"_s);
    }

    void isUnderMeansInsideButNotTheSame()
    {
        Fixture_ f{};
        auto base = f.key(u"a"_s);

        QVERIFY(f.key(u"a/notes.txt"_s).isUnder(base));
        QVERIFY(f.key(u"a/b/notes.txt"_s).isUnder(base));
        QVERIFY(!base.isUnder(base));
        QVERIFY(base.isAtOrUnder(base));
        QVERIFY(!f.key(u"ab/notes.txt"_s).isUnder(base));
        QVERIFY(!f.key(u"notes.txt"_s).isUnder(base));
        QVERIFY(!f.key(u"a/notes.txt"_s).isUnder(CanonicalPath()));
        QVERIFY(!CanonicalPath().isAtOrUnder(CanonicalPath()));
    }

    void relativeToGivesThePathBelowTheBase()
    {
        Fixture_ f{};
        auto base = f.key(u"a"_s);

        QCOMPARE(
            f.key(u"a/b/Notes.txt"_s).relativeTo(base),
            Coco::Path(u"b/Notes.txt"_s));
        QVERIFY(base.relativeTo(base).isEmpty());
        QVERIFY(f.key(u"c/notes.txt"_s).relativeTo(base).isEmpty());
    }

    void rebasedFollowsARenamedFolder()
    {
        Fixture_ f{};
        auto old_base = f.key(u"old"_s);
        auto new_base = f.key(u"new"_s);

        QVERIFY(
            f.key(u"old/sub/notes.txt"_s).rebased(old_base, new_base) ==
            f.key(u"new/sub/notes.txt"_s));
        QVERIFY(old_base.rebased(old_base, new_base) == new_base);
        QVERIFY(
            f.key(u"other/notes.txt"_s).rebased(old_base, new_base).isEmpty());
    }

    void anySpellingFindsTheSameHashEntry()
    {
        Fixture_ f{};
        f.makeFolder(u"sub"_s);

        QHash<CanonicalPath, int> hash{};
        hash.insert(f.key(u"notes.txt"_s), 1);

        QCOMPARE(hash.value(f.key(u"sub/../notes.txt"_s)), 1);
    }
};

QTEST_GUILESS_MAIN(CanonicalPathTest)
#include "CanonicalPathTest.moc"
