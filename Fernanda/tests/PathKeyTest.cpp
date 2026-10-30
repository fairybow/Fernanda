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

#include <QDir>
#include <QFile>
#include <QHash>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QtGlobal>

#include <Coco/Path.h>

#include "core/PathKey.h"

using namespace Qt::StringLiterals;

using Fernanda::PathKey;

// How a path becomes a key that is the same for every spelling of one path
// (core/PathKey.h): normalizing, letter case, containment, and re-keying under
// a renamed folder. Keys are made from the path alone, so only the tests about
// the file system's part (a file created later, a symlink) make real files.
//
// Each test works in its own temporary folder
class PathKeyTest : public QObject
{
    Q_OBJECT

private:
    // A temporary folder. Paths given to these functions are relative to it
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

        [[nodiscard]] PathKey key(const QString& relative) const
        {
            return PathKey::of(absolute(relative));
        }

        // Creates the folders above the file too
        void write(const QString& relative) const
        {
            auto path = absolute(relative);
            QVERIFY(QDir().mkpath(path.parent().toQString()));

            QFile file(path.toQString());
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
    };

private slots:
    void emptyAndRelativePathsGiveAnEmptyKey()
    {
        QVERIFY(PathKey::of({}).isEmpty());
        QVERIFY(PathKey::of(Coco::Path(u"a/b.txt"_s)).isEmpty());
        QVERIFY(PathKey().isEmpty());
    }

    void dotAndDotDotGiveTheSameKeyAsThePlainPath()
    {
        Fixture_ f{};
        auto plain = f.key(u"notes.txt"_s);

        QVERIFY(!plain.isEmpty());
        QVERIFY(f.key(u"sub/../notes.txt"_s) == plain);
        QVERIFY(f.key(u"./notes.txt"_s) == plain);
        QVERIFY(f.key(u"a/b/../../notes.txt"_s) == plain);
    }

    void separatorsGiveTheSameKey()
    {
        Fixture_ f{};

        QVERIFY(f.key(u"a//b.txt"_s) == f.key(u"a/b.txt"_s));
        QVERIFY(f.key(u"folder/"_s) == f.key(u"folder"_s));

#if defined(Q_OS_WIN)

        QVERIFY(f.key(u"a\\b.txt"_s) == f.key(u"a/b.txt"_s));

#endif
    }

    // So a key's path reads the same on every platform, in logs and in
    // anything built from it
    void thePathIsWrittenWithForwardSlashes()
    {
        Fixture_ f{};
        auto key = f.key(u"a/b/notes.txt"_s);

        QVERIFY(!key.path().toQString().contains(u'\\'));
        QVERIFY(!key.path().toQString().endsWith(u'/'));
        QCOMPARE(key.path().nameQString(), u"notes.txt"_s);
    }

    void aFileCreatedLaterKeepsItsKey()
    {
        Fixture_ f{};
        auto before = f.key(u"a/new.txt"_s);

        f.write(u"a/new.txt"_s);
        auto after = f.key(u"a/new.txt"_s);

        QVERIFY(before == after);
        QCOMPARE(qHash(before), qHash(after));
    }

    // Symlinks aren't followed: a folder linked into a vault keeps its files'
    // paths inside the vault
    void aSymlinkedFolderIsNotFollowed()
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

        auto through_link = f.key(u"link/notes.txt"_s);

        QVERIFY(through_link != f.key(u"real/notes.txt"_s));
        QVERIFY(through_link.isUnder(f.key({})));

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

        QVERIFY(lower != upper);

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
        QVERIFY(!f.key(u"a/../notes.txt"_s).isUnder(base));
        QVERIFY(!f.key(u"a/notes.txt"_s).isUnder(PathKey()));
        QVERIFY(!PathKey().isAtOrUnder(PathKey()));
    }

    // A root's own spelling ends in a separator ("/", "C:/"), which the
    // comparison has to allow for
    void aFileIsUnderTheFileSystemRoot()
    {
        auto root = PathKey::of(Coco::Path::Root());
        auto file = PathKey::of(Coco::Path::Root() / Coco::Path(u"a/b.txt"_s));

        QVERIFY(!root.isEmpty());
        QVERIFY(file.isUnder(root));
        QCOMPARE(file.relativeTo(root), Coco::Path(u"a/b.txt"_s));
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

    // Where case is ignored the two bases are equal keys, but the files below
    // still take the new spelling
    void rebasedFollowsACaseOnlyRename()
    {
        Fixture_ f{};
        auto rebased = f.key(u"notes/a.txt"_s)
                           .rebased(f.key(u"notes"_s), f.key(u"Notes"_s));

        QCOMPARE(rebased.path().parent().nameQString(), u"Notes"_s);
        QCOMPARE(rebased.path().nameQString(), u"a.txt"_s);
    }

    void anySpellingFindsTheSameHashEntry()
    {
        Fixture_ f{};

        QHash<PathKey, int> hash{};
        hash.insert(f.key(u"notes.txt"_s), 1);

        QCOMPARE(hash.value(f.key(u"sub/../notes.txt"_s)), 1);
    }
};

QTEST_GUILESS_MAIN(PathKeyTest)

#include "PathKeyTest.moc"
