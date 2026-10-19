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

#include <QAbstractItemModelTester>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLocale>
#include <QModelIndex>
#include <QObject>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/VaultTreeModel.h"

using namespace Qt::StringLiterals;
using Fernanda::BufferStore;
using Fernanda::PathKey;
using Fernanda::Vault;
using Fernanda::VaultTreeModel;

// The file tree every view of a vault shares (core/VaultTreeModel.h): what it
// lists and in what order, how it follows the vault's own file operations,
// and how it follows changes made by other programs.
//
// Each test gets its own vault in a temporary folder, and reaches the model
// through it, as the app does. Most also run with Qt's model tester attached:
// it watches every signal the model sends and fails the test if a row is
// announced that isn't there, or changes without being announced. The tester
// reads the whole tree when attached, which lists every folder, so the tests
// about folders nobody has opened run without it.
//
// The last section waits on the file watcher, so it takes real time. See
// BufferStoreTimedTest.cpp for how those waits are written.
//
// Qt Test runs every private slot as a test
class VaultTreeModelTest : public QObject
{
    Q_OBJECT

private:
    // Longer than the model's wait before it handles folder changes, with
    // room for the watcher to report them
    static constexpr int SETTLE_MS_ = 700;

    // A folder of files, then a vault on it. Files are written by the make
    // functions before the vault exists; ones a test adds later are "outside"
    // changes as far as the vault can tell.
    //
    // The vault is made by start(), after the first files, and is destroyed
    // before the store it's lent, and that before the folder
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };
        BufferStore store{ nullptr };
        Vault* vault = nullptr;
        VaultTreeModel* model = nullptr;
        QAbstractItemModelTester* tester = nullptr;

        Fixture_() = default;
        Fixture_(const Fixture_&) = delete;
        Fixture_& operator=(const Fixture_&) = delete;

        ~Fixture_()
        {
            delete tester;
            delete vault;
        }

        void start()
        {
            startWithoutTester();
            tester = new QAbstractItemModelTester(
                model,
                QAbstractItemModelTester::FailureReportingMode::QtTest);
        }

        // Leaves every folder but the root unlisted
        void startWithoutTester()
        {
            vault = new Vault(root, &store, nullptr);
            model = vault->treeModel();
        }

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

        // An empty path is the vault's root
        [[nodiscard]] PathKey key(const QString& relative = {}) const
        {
            return PathKey::of(absolute(relative));
        }

        void makeFolder(const QString& relative) const
        {
            QDir(folder.path()).mkpath(relative);
        }

        // Creates the folders above the file too
        void makeFile(const QString& relative) const
        {
            QFileInfo info(folder.filePath(relative));
            QDir().mkpath(info.path());

            QFile file(info.filePath());
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("text");
        }

        // The rows under a folder, as the tree shows them, top to bottom.
        // Lists the folder first if nothing has yet. An empty path is the
        // vault's root
        [[nodiscard]] QStringList rows(const QString& relative = {}) const
        {
            auto parent = relative.isEmpty()
                              ? QModelIndex{}
                              : model->indexOf(absolute(relative));

            if (model->canFetchMore(parent)) {
                model->fetchMore(parent);
            }

            QStringList shown{};

            for (auto row = 0; row < model->rowCount(parent); ++row) {
                shown << model->index(row, 0, parent).data().toString();
            }

            return shown;
        }

        [[nodiscard]] QModelIndex index(const QString& relative) const
        {
            return model->indexOf(absolute(relative));
        }
    };

    // The rows under a folder of some other vault's tree, as rows() gives
    // them for the fixture's. Lists the folder first if nothing has yet
    [[nodiscard]] static QStringList
    rowsOf_(VaultTreeModel* model, const QModelIndex& parent)
    {
        if (model->canFetchMore(parent)) {
            model->fetchMore(parent);
        }

        QStringList shown{};

        for (auto row = 0; row < model->rowCount(parent); ++row) {
            shown << model->index(row, 0, parent).data().toString();
        }

        return shown;
    }

private slots:
    // The model sorts by the default locale's rules. Under the "C" locale,
    // which a build server may run with, Qt compares names as plain strings
    // and a number inside a name isn't read as one. A named locale here keeps
    // the order the same on every machine
    void initTestCase()
    {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    }

    // --- Listing ------------------------------------------------------------

    // Folders first, then files, each by name: letter case ignored, and a
    // number inside a name read as a number
    void rowsAreFoldersFirstThenByName()
    {
        Fixture_ f{};
        f.makeFile(u"cherry.md"_s);
        f.makeFile(u"banana.txt"_s);
        f.makeFile(u"Apple.txt"_s);
        f.makeFile(u"chapter 10.txt"_s);
        f.makeFile(u"chapter 2.txt"_s);
        f.makeFolder(u"notes"_s);
        f.makeFolder(u"Drafts"_s);
        f.start();

        QCOMPARE(
            f.rows(),
            QStringList(
                { u"Drafts"_s,
                  u"notes"_s,
                  u"Apple"_s,
                  u"banana"_s,
                  u"chapter 2"_s,
                  u"chapter 10"_s,
                  u"cherry"_s }));
    }

    // A file shows without its extension. A folder shows whole, since a
    // period in a folder's name isn't one
    void filesShowWithoutTheirExtension()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.tar.md"_s);
        f.makeFolder(u"Draft v1.0"_s);
        f.start();

        QCOMPARE(
            f.rows(),
            QStringList({ u"Draft v1.0"_s, u"a"_s, u"b.tar"_s }));

        // The whole name is still there to be asked for
        QCOMPARE(f.model->nameOf(f.index(u"a.txt"_s)), Coco::Path("a.txt"));
    }

    void hiddenAndUnsupportedEntriesAreNotListed()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.docx"_s);
        f.makeFile(u"Makefile"_s);
        f.makeFile(u".hidden.txt"_s);
        f.makeFile(u".git/c.txt"_s);
        f.makeFolder(u".cache"_s);
        f.start();

        // The vault's own .fernanda folder is hidden like any other
        QCOMPARE(f.rows(), QStringList({ u"a"_s }));
    }

    // An unsupported file is left out, but a folder holding only such files
    // is still a folder
    void aFolderOfUnlistedFilesIsListedAndEmpty()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.docx"_s);
        f.start();

        QCOMPARE(f.rows(), QStringList({ u"one"_s }));
        QCOMPARE(f.rows(u"one"_s), QStringList{});
    }

    void anEmptyVaultHasNoRows()
    {
        Fixture_ f{};
        f.start();

        QCOMPARE(f.rows(), QStringList{});
    }

    // --- Indexes, paths, and kinds ------------------------------------------

    void theInvalidIndexIsTheRoot()
    {
        Fixture_ f{};
        f.start();

        QCOMPARE(f.model->pathOf(QModelIndex{}), f.root);
        QVERIFY(f.model->isDir(QModelIndex{}));
        QCOMPARE(f.model->root(), f.root);
    }

    void anIndexKnowsItsPathAndKind()
    {
        Fixture_ f{};
        f.makeFile(u"one/two/a.txt"_s);
        f.start();

        auto folder = f.index(u"one/two"_s);
        auto file = f.index(u"one/two/a.txt"_s);

        QVERIFY(folder.isValid());
        QVERIFY(file.isValid());

        QCOMPARE(f.model->pathOf(folder), f.absolute(u"one/two"_s));
        QCOMPARE(f.model->pathOf(file), f.absolute(u"one/two/a.txt"_s));
        QVERIFY(f.model->isDir(folder));
        QVERIFY(!f.model->isDir(file));
        QCOMPARE(file.parent(), folder);
        QCOMPARE(folder.parent(), f.index(u"one"_s));
    }

    void indexOfIsInvalidForWhatIsNotShown()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.docx"_s);
        f.makeFile(u".git/c.txt"_s);
        f.start();

        QVERIFY(f.index(u"a.txt"_s).isValid());

        QVERIFY(!f.model->indexOf(f.root).isValid());
        QVERIFY(!f.index(u"gone.txt"_s).isValid());
        QVERIFY(!f.index(u"b.docx"_s).isValid());
        QVERIFY(!f.index(u".git"_s).isValid());
        QVERIFY(!f.index(u".git/c.txt"_s).isValid());
        QVERIFY(!f.index(u"a.txt/under-a-file"_s).isValid());
        QVERIFY(!f.model->indexOf(f.root.parent()).isValid());
    }

    void nothingIsEditableAndFilesHaveNoChildren()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.txt"_s);
        f.start();

        auto folder = f.index(u"one"_s);
        auto file = f.index(u"one/a.txt"_s);

        QVERIFY(!(f.model->flags(folder) & Qt::ItemIsEditable));
        QVERIFY(!(f.model->flags(file) & Qt::ItemIsEditable));
        QVERIFY(f.model->flags(file) & Qt::ItemNeverHasChildren);
        QVERIFY(!(f.model->flags(folder) & Qt::ItemNeverHasChildren));
        QVERIFY(!f.model->hasChildren(file));
        QCOMPARE(f.model->rowCount(file), 0);
    }

    // --- Listing a folder only when asked -----------------------------------

    // A folder is read from disk the first time something asks for what is
    // in it. Until then it claims to have children, so its arrow shows
    void aFolderIsListedWhenFirstAskedFor()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.txt"_s);
        f.makeFolder(u"empty"_s);
        f.startWithoutTester();

        // The root is listed with the vault. This looks only at the root's
        // rows, so it lists nothing more
        QModelIndex one{};
        QModelIndex empty{};

        for (auto row = 0; row < f.model->rowCount(QModelIndex{}); ++row) {
            auto index = f.model->index(row, 0, QModelIndex{});

            if (index.data().toString() == u"one"_s) {
                one = index;
            } else if (index.data().toString() == u"empty"_s) {
                empty = index;
            }
        }

        QVERIFY(one.isValid());
        QVERIFY(empty.isValid());

        QVERIFY(f.model->canFetchMore(one));
        QVERIFY(f.model->hasChildren(one));
        QCOMPARE(f.model->rowCount(one), 0);

        // An empty folder makes the same claim until it is listed
        QVERIFY(f.model->hasChildren(empty));

        f.model->fetchMore(one);
        f.model->fetchMore(empty);

        QVERIFY(!f.model->canFetchMore(one));
        QCOMPARE(f.model->rowCount(one), 1);
        QVERIFY(f.model->hasChildren(one));
        QVERIFY(!f.model->hasChildren(empty));
    }

    // --- The vault's own operations -----------------------------------------

    void aCreatedFileAppearsInPlace()
    {
        Fixture_ f{};
        f.makeFile(u"Apple.txt"_s);
        f.makeFile(u"zebra.txt"_s);
        f.start();

        QSignalSpy inserted(f.model, &VaultTreeModel::rowsInserted);

        QCOMPARE(f.vault->createFile(f.key()), f.key(u"Untitled.txt"_s));

        QCOMPARE(inserted.count(), 1);
        QCOMPARE(
            f.rows(),
            QStringList({ u"Apple"_s, u"Untitled"_s, u"zebra"_s }));
    }

    void aCreatedFolderAppearsAmongTheFolders()
    {
        Fixture_ f{};
        f.makeFile(u"Apple.txt"_s);
        f.makeFolder(u"zeta"_s);
        f.start();

        f.vault->createFolder(f.key());

        QCOMPARE(
            f.rows(),
            QStringList({ u"Untitled"_s, u"zeta"_s, u"Apple"_s }));
    }

    void aCreatedFileAppearsInAListedSubfolder()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s }));

        f.vault->createFile(f.key(u"one"_s));

        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s, u"Untitled"_s }));
    }

    // The new folder is one row, holding the rest of the path, which is listed
    // when first opened. Without the tester, which would list the new folder
    // from inside the signal announcing it
    void aFileCreatedInNewFoldersAppearsWithThem()
    {
        Fixture_ f{};
        f.makeFile(u"Apple.txt"_s);
        f.startWithoutTester();

        QSignalSpy inserted(f.model, &VaultTreeModel::rowsInserted);

        QVERIFY(f.vault->createFileAt(f.key(u"new/deeper/a.txt"_s)));

        QCOMPARE(inserted.count(), 1);
        QCOMPARE(f.rows(), QStringList({ u"new"_s, u"Apple"_s }));
        QCOMPARE(f.rows(u"new"_s), QStringList({ u"deeper"_s }));
        QCOMPARE(f.rows(u"new/deeper"_s), QStringList({ u"a"_s }));
    }

    void aFileCreatedAtAPathAppearsInAListedFolder()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s }));

        QVERIFY(f.vault->createFileAt(f.key(u"one/b.txt"_s)));

        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s, u"b"_s }));
    }

    // The row is the same row afterward: a view keeps it selected, and keeps
    // a folder's children expanded
    void aRenamedFileKeepsItsRowAndMovesToItsPlace()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"m.txt"_s);
        f.makeFile(u"z.txt"_s);
        f.start();

        QPersistentModelIndex a(f.index(u"a.txt"_s));
        QSignalSpy moved(f.model, &VaultTreeModel::rowsMoved);

        f.vault->rename(f.key(u"a.txt"_s), u"x.txt"_s);

        QCOMPARE(f.rows(), QStringList({ u"m"_s, u"x"_s, u"z"_s }));
        QCOMPARE(moved.count(), 1);

        QVERIFY(a.isValid());
        QCOMPARE(a.row(), 1);
        QCOMPARE(a.data().toString(), u"x"_s);
        QCOMPARE(f.model->pathOf(a), f.absolute(u"x.txt"_s));
    }

    // A rename that leaves the row where it is moves nothing
    void aRenameThatKeepsItsPlaceOnlyChangesTheName()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"m.txt"_s);
        f.makeFile(u"z.txt"_s);
        f.start();

        QPersistentModelIndex m(f.index(u"m.txt"_s));
        QSignalSpy moved(f.model, &VaultTreeModel::rowsMoved);
        QSignalSpy changed(f.model, &VaultTreeModel::dataChanged);

        f.vault->rename(f.key(u"m.txt"_s), u"n.txt"_s);

        QCOMPARE(f.rows(), QStringList({ u"a"_s, u"n"_s, u"z"_s }));
        QCOMPARE(moved.count(), 0);
        QCOMPARE(changed.count(), 1);
        QVERIFY(m.isValid());
        QCOMPARE(m.data().toString(), u"n"_s);
    }

    // Renaming to each end of the list: the row counts Qt's move signals
    // want differ for a move up and a move down
    void aRenameToEitherEndOfTheList()
    {
        Fixture_ f{};
        f.makeFile(u"b.txt"_s);
        f.makeFile(u"m.txt"_s);
        f.makeFile(u"y.txt"_s);
        f.start();

        f.vault->rename(f.key(u"b.txt"_s), u"z.txt"_s);
        QCOMPARE(f.rows(), QStringList({ u"m"_s, u"y"_s, u"z"_s }));

        f.vault->rename(f.key(u"z.txt"_s), u"a.txt"_s);
        QCOMPARE(f.rows(), QStringList({ u"a"_s, u"m"_s, u"y"_s }));
    }

    // A renamed folder is the same row, and everything listed inside it
    // comes along
    void aRenamedFolderKeepsItsListedChildren()
    {
        Fixture_ f{};
        f.makeFile(u"one/two/a.txt"_s);
        f.makeFolder(u"zeta"_s);
        f.start();

        QPersistentModelIndex one(f.index(u"one"_s));
        QPersistentModelIndex a(f.index(u"one/two/a.txt"_s));

        f.vault->rename(f.key(u"one"_s), u"uno"_s);

        QCOMPARE(f.rows(), QStringList({ u"uno"_s, u"zeta"_s }));
        QVERIFY(one.isValid());
        QVERIFY(a.isValid());
        QCOMPARE(f.model->pathOf(a), f.absolute(u"uno/two/a.txt"_s));

        // Still listed: nothing needs fetching again
        QVERIFY(!f.model->canFetchMore(one));
        QCOMPARE(f.rows(u"uno/two"_s), QStringList({ u"a"_s }));
    }

    // A rename to a name the tree doesn't show takes the row away
    void aRenameToAnUnshownNameRemovesTheRow()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.txt"_s);
        f.start();

        QPersistentModelIndex a(f.index(u"a.txt"_s));

        f.vault->rename(f.key(u"a.txt"_s), u"a.docx"_s);

        QCOMPARE(f.rows(), QStringList({ u"b"_s }));
        QVERIFY(!a.isValid());
    }

    void aMovedFileChangesParentAndKeepsItsRow()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"one/b.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"b"_s }));

        QPersistentModelIndex a(f.index(u"a.txt"_s));

        f.vault->move(f.key(u"a.txt"_s), f.key(u"one"_s));

        QCOMPARE(f.rows(), QStringList({ u"one"_s }));
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s, u"b"_s }));

        QVERIFY(a.isValid());
        QCOMPARE(a.parent(), f.index(u"one"_s));
        QCOMPARE(f.model->pathOf(a), f.absolute(u"one/a.txt"_s));
    }

    // A folder nobody has opened has no rows to add one to. The moved row
    // goes, and the folder lists it with the rest when first opened
    void aMoveIntoAnUnlistedFolderRemovesTheRow()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"one/b.txt"_s);
        f.startWithoutTester();

        QPersistentModelIndex a(f.index(u"a.txt"_s));

        f.vault->move(f.key(u"a.txt"_s), f.key(u"one"_s));

        QVERIFY(!a.isValid());
        QCOMPARE(f.rows(), QStringList({ u"one"_s }));
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s, u"b"_s }));
    }

    void aMovedFolderKeepsItsListedChildren()
    {
        Fixture_ f{};
        f.makeFile(u"one/two/a.txt"_s);
        f.makeFolder(u"other"_s);
        f.start();
        QCOMPARE(f.rows(u"other"_s), QStringList{});

        QPersistentModelIndex a(f.index(u"one/two/a.txt"_s));

        f.vault->move(f.key(u"one"_s), f.key(u"other"_s));

        QCOMPARE(f.rows(), QStringList({ u"other"_s }));
        QCOMPARE(f.rows(u"other"_s), QStringList({ u"one"_s }));
        QVERIFY(a.isValid());
        QCOMPARE(f.model->pathOf(a), f.absolute(u"other/one/two/a.txt"_s));
    }

    // A refused operation leaves the tree as it was
    void aRefusedRenameChangesNothing()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.txt"_s);
        f.start();

        QSignalSpy moved(f.model, &VaultTreeModel::rowsMoved);
        QSignalSpy removed(f.model, &VaultTreeModel::rowsRemoved);
        QSignalSpy changed(f.model, &VaultTreeModel::dataChanged);

        QVERIFY(f.vault->rename(f.key(u"a.txt"_s), u"b.txt"_s).isEmpty());

        QCOMPARE(f.rows(), QStringList({ u"a"_s, u"b"_s }));
        QCOMPARE(moved.count() + removed.count() + changed.count(), 0);
    }

    // What the vault calls after a file leaves disk
    void aRemovedEntryLeavesTheTree()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"one/two/b.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one/two"_s), QStringList({ u"b"_s }));

        QPersistentModelIndex b(f.index(u"one/two/b.txt"_s));

        f.model->releaseWatches(f.absolute(u"one"_s));
        QVERIFY(QDir(f.folder.filePath(u"one"_s)).removeRecursively());
        f.model->applyRemoval(f.absolute(u"one"_s));

        QCOMPARE(f.rows(), QStringList({ u"a"_s }));
        QVERIFY(!b.isValid());
    }

    // --- Changes made by another program ------------------------------------

    void anOutsideFileAppears()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.start();

        f.makeFile(u"b.txt"_s);

        QTRY_COMPARE(f.rows(), QStringList({ u"a"_s, u"b"_s }));
    }

    // Rows that didn't change are the same rows afterward, so a view keeps
    // its selection and its open folders
    void anOutsideChangeKeepsTheOtherRows()
    {
        Fixture_ f{};
        f.makeFile(u"one/a.txt"_s);
        f.makeFile(u"z.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one"_s), QStringList({ u"a"_s }));

        QPersistentModelIndex one(f.index(u"one"_s));
        QPersistentModelIndex a(f.index(u"one/a.txt"_s));
        QPersistentModelIndex z(f.index(u"z.txt"_s));

        f.makeFile(u"m.txt"_s);

        QTRY_COMPARE(f.rows(), QStringList({ u"one"_s, u"m"_s, u"z"_s }));

        QVERIFY(one.isValid());
        QVERIFY(a.isValid());
        QVERIFY(z.isValid());
        QCOMPARE(z.row(), 2);
        QVERIFY(!f.model->canFetchMore(one));
    }

    void anOutsideDeleteRemovesTheRow()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"b.txt"_s);
        f.start();

        QVERIFY(QFile::remove(f.folder.filePath(u"a.txt"_s)));

        QTRY_COMPARE(f.rows(), QStringList({ u"b"_s }));
    }

    void anOutsideRenameShowsAsTheNewName()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"m.txt"_s);
        f.start();

        QVERIFY(
            QFile::rename(
                f.folder.filePath(u"a.txt"_s),
                f.folder.filePath(u"z.txt"_s)));

        QTRY_COMPARE(f.rows(), QStringList({ u"m"_s, u"z"_s }));
    }

    void anOutsideChangeInAListedSubfolderShows()
    {
        Fixture_ f{};
        f.makeFile(u"one/two/a.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one/two"_s), QStringList({ u"a"_s }));

        f.makeFile(u"one/two/b.txt"_s);

        QTRY_COMPARE(f.rows(u"one/two"_s), QStringList({ u"a"_s, u"b"_s }));
    }

    void anOutsideFolderDeleteRemovesItsRows()
    {
        Fixture_ f{};
        f.makeFile(u"one/two/a.txt"_s);
        f.makeFile(u"z.txt"_s);
        f.start();
        QCOMPARE(f.rows(u"one/two"_s), QStringList({ u"a"_s }));

        // Each listed folder is watched, and on Windows a watch is an open
        // handle that can stop another program deleting the folder. If the
        // delete is refused there is nothing to observe
        if (!QDir(f.folder.filePath(u"one"_s)).removeRecursively()) {
            QSKIP("The watched folder couldn't be deleted from outside");
        }

        QTRY_COMPARE(f.rows(), QStringList({ u"z"_s }));
    }

    void anOutsideHiddenOrUnsupportedFileAddsNoRow()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.start();

        QSignalSpy inserted(f.model, &VaultTreeModel::rowsInserted);

        f.makeFile(u".hidden.txt"_s);
        f.makeFile(u"b.docx"_s);
        QTest::qWait(SETTLE_MS_);

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(f.rows(), QStringList({ u"a"_s }));
    }

    // The vault's own rename updates the tree at once, and the watcher then
    // reports the same change. Taking that in a second time changes nothing
    void theWatchersReportOfOwnRenameChangesNothing()
    {
        Fixture_ f{};
        f.makeFile(u"a.txt"_s);
        f.makeFile(u"m.txt"_s);
        f.start();

        f.vault->rename(f.key(u"a.txt"_s), u"z.txt"_s);
        QPersistentModelIndex z(f.index(u"z.txt"_s));

        QSignalSpy inserted(f.model, &VaultTreeModel::rowsInserted);
        QSignalSpy removed(f.model, &VaultTreeModel::rowsRemoved);

        QTest::qWait(SETTLE_MS_);

        QCOMPARE(inserted.count() + removed.count(), 0);
        QVERIFY(z.isValid());
        QCOMPARE(f.rows(), QStringList({ u"m"_s, u"z"_s }));
    }

    // --- A vault inside the fixture's ---------------------------------------
    //
    // The inner vault is lent the fixture's store, as App lends its one store
    // to every vault, and is made after the fixture, so it's destroyed first

    // The inner vault's tree moves the same row, as the outer's does, so a
    // window showing it keeps the folder and what's in it open
    void aRenameThroughAnOuterVaultMovesTheInnerTreesRow()
    {
        Fixture_ f{};
        f.makeFile(u"inner/sub/a.txt"_s);
        f.start();

        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);
        auto* inner_model = inner.treeModel();
        QCOMPARE(rowsOf_(inner_model, {}), QStringList({ u"sub"_s }));

        QPersistentModelIndex sub(
            inner_model->indexOf(f.absolute(u"inner/sub"_s)));
        QCOMPARE(rowsOf_(inner_model, sub), QStringList({ u"a"_s }));

        QVERIFY(
            !f.vault->rename(f.key(u"inner/sub"_s), u"renamed"_s).isEmpty());

        QVERIFY(sub.isValid());
        QCOMPARE(sub.data().toString(), u"renamed"_s);
        QVERIFY(!inner_model->canFetchMore(sub));
        QCOMPARE(rowsOf_(inner_model, sub), QStringList({ u"a"_s }));
        QCOMPARE(rowsOf_(inner_model, {}), QStringList({ u"renamed"_s }));

        // The watcher's later report of the same rename changes nothing
        QTest::qWait(SETTLE_MS_);
        QVERIFY(sub.isValid());
        QCOMPARE(rowsOf_(inner_model, {}), QStringList({ u"renamed"_s }));
    }

    // After a trash the entry's row leaves every tree. The trash itself needs
    // a system trash, so the store is called as Vault::moveToTrash calls it
    // once the disk op is done
    void aRemovalThroughTheStoreLeavesEveryTree()
    {
        Fixture_ f{};
        f.makeFile(u"inner/sub/a.txt"_s);
        f.makeFile(u"inner/b.txt"_s);
        f.start();

        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);
        auto* inner_model = inner.treeModel();
        QCOMPARE(rowsOf_(inner_model, {}), QStringList({ u"sub"_s, u"b"_s }));
        QCOMPARE(f.rows(u"inner"_s), QStringList({ u"sub"_s, u"b"_s }));

        QVERIFY(QDir(f.folder.filePath(u"inner/sub"_s)).removeRecursively());
        f.store.applyRemoval(f.key(u"inner/sub"_s));

        QCOMPARE(rowsOf_(inner_model, {}), QStringList({ u"b"_s }));
        QCOMPARE(f.rows(u"inner"_s), QStringList({ u"b"_s }));
    }

    // The outer vault's rename lets go of the inner tree's watches inside the
    // folder, and a failed one gives them back: a later change inside the
    // folder still shows in the inner tree
    void aFailedRenameThroughAnOuterVaultLeavesTheInnerTreeWatching()
    {
        Fixture_ f{};
        f.makeFile(u"inner/sub/a.txt"_s);
        f.makeFile(u"inner/taken.txt"_s);
        f.start();

        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);
        auto* inner_model = inner.treeModel();
        rowsOf_(inner_model, {});
        auto sub = inner_model->indexOf(f.absolute(u"inner/sub"_s));
        QCOMPARE(rowsOf_(inner_model, sub), QStringList({ u"a"_s }));

        QVERIFY(
            f.vault->rename(f.key(u"inner/sub"_s), u"taken.txt"_s).isEmpty());

        f.makeFile(u"inner/sub/b.txt"_s);

        QTRY_COMPARE(
            rowsOf_(inner_model, sub),
            QStringList({ u"a"_s, u"b"_s }));
    }
};

QTEST_MAIN(VaultTreeModelTest)

#include "VaultTreeModelTest.moc"
