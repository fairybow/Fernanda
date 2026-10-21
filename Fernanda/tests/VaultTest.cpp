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
#include <QIODevice>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "models/AbstractFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using Fernanda::AbstractFileModel;
using Fernanda::BufferStore;
using Fernanda::PathKey;
using Fernanda::TextFileModel;
using Fernanda::Vault;
using Fernanda::VaultConfig;

// What a Vault does with the files inside it (core/Vault.h): creating,
// renaming, moving, and trashing, with the open buffers following, and its
// settings and dictionary. The buffers themselves are BufferStoreTest's.
//
// Each test gets its own vault in a temporary folder, with its own store.
//
// Qt Test runs every private slot as a test
class VaultTest : public QObject
{
    Q_OBJECT

private:
    // A vault in a temporary folder, the store it's lent, and the file work a
    // test does around them. Paths given to these functions are relative to
    // the vault's root.
    //
    // Declared in the order they're made, so each is destroyed before what it
    // depends on
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };
        BufferStore store{ nullptr };
        Vault vault{ root, &store, nullptr };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

        // The key of a path written relative to the root. Not checked, so a
        // test can name a path outside the vault ("../x") to see it refused
        [[nodiscard]] PathKey key(const QString& relative) const
        {
            return PathKey::of(absolute(relative));
        }

        [[nodiscard]] PathKey rootKey() const { return vault.rootKey(); }

        void makeFolder(const QString& relative) const
        {
            QDir(folder.path()).mkpath(relative);
        }

        // Creates the folders above the file too
        void write(const QString& relative, const QByteArray& bytes) const
        {
            QFileInfo info(folder.filePath(relative));
            QDir().mkpath(info.path());

            QFile file(info.filePath());
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(bytes);
        }

        [[nodiscard]] QByteArray read(const QString& relative) const
        {
            QFile file(folder.filePath(relative));

            if (!file.open(QIODevice::ReadOnly)) {
                return {};
            }

            return file.readAll();
        }

        [[nodiscard]] bool exists(const QString& relative) const
        {
            return QFileInfo::exists(folder.filePath(relative));
        }

        // The entry's name as the folder lists it, so a change of letter case
        // shows on a filesystem that ignores case
        [[nodiscard]] QStringList namesIn(const QString& relative) const
        {
            return QDir(folder.filePath(relative))
                .entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        }

        // Opens a file that needs no question asked. The test fails if the
        // store asks one
        [[nodiscard]] AbstractFileModel* open(const PathKey& key)
        {
            auto asked = false;

            auto* model = store.openModel(
                key,
                [&asked](const PathKey&, const QByteArray&) {
                    asked = true;
                    return false;
                });

            if (asked) {
                QTest::qFail(
                    "The store asked about a lossy open",
                    __FILE__,
                    __LINE__);
            }

            return model;
        }

        [[nodiscard]] AbstractFileModel* open(const QString& relative)
        {
            return open(key(relative));
        }
    };

    // Types at the end of a text buffer, as a view's editor would
    static void type_(AbstractFileModel* model, const QString& text)
    {
        auto* text_model = qobject_cast<TextFileModel*>(model);
        QVERIFY(text_model);

        QTextDocument document{};
        document.setDocumentLayout(new QPlainTextDocumentLayout(&document));
        text_model->registerView(&document);

        QTextCursor cursor(&document);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
    }

private slots:
    // --- Paths --------------------------------------------------------------

    void containsIsByFolderNotByPrefix()
    {
        Fixture_ f{};

        QVERIFY(f.vault.contains(f.rootKey()));
        QVERIFY(f.vault.contains(f.key(u"a.txt"_s)));
        QVERIFY(f.vault.contains(f.key(u"one/two/a.txt"_s)));

        // A sibling folder whose name starts with the vault's name
        QVERIFY(!f.vault.contains(
            PathKey::of(Coco::Path(f.folder.path() + u"x"_s))));
        QVERIFY(!f.vault.contains(PathKey::of(f.root.parent())));
    }

    // The vault's root is spelled as keys spell paths, so the keys it hands
    // out start with it
    void theRootIsTheKeysPath()
    {
        Fixture_ f{};

        QCOMPARE(f.vault.root(), f.rootKey().path());
        QVERIFY(f.vault.createFile(f.rootKey()).path().isUnder(f.vault.root()));
    }

    // A key for another folder is refused, and the vault's own spelling again
    // is no change: neither touches the root or announces anything
    void respellRootRefusesAnotherFolder()
    {
        Fixture_ f{};
        auto root = f.vault.root();
        QSignalSpy respelled(&f.vault, &Vault::rootRespelled);

        f.vault.respellRoot(f.key(u"sub"_s));
        f.vault.respellRoot(PathKey::of(root.parent()));
        f.vault.respellRoot(PathKey{});
        f.vault.respellRoot(f.rootKey());

        QCOMPARE(f.vault.root(), root);
        QCOMPARE(f.vault.treeModel()->root(), root);
        QCOMPARE(respelled.count(), 0);
    }

    // The vault's own folder is made with the vault
    void constructionMakesTheDotFolder()
    {
        Fixture_ f{};

        QVERIFY(QFileInfo(f.folder.filePath(u".fernanda"_s)).isDir());
    }

    // What the vault lists is what it will open, and nothing hidden
    void visibleFilesLeavesOutHiddenAndUnsupported()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"one/b.md"_s, "b");
        f.write(u"one/two/c.pdf"_s, "c");
        f.write(u"d.docx"_s, "d");
        f.write(u".hidden.txt"_s, "e");
        f.write(u".git/f.txt"_s, "f");
        f.write(u"one/.cache/g.txt"_s, "g");

        QStringList listed{};

        for (const auto& key : f.vault.visibleFiles()) {
            listed << key.relativeTo(f.rootKey()).prettyQString();
        }

        listed.sort();

        QCOMPARE(
            listed,
            QStringList({ u"a.txt"_s, u"one/b.md"_s, u"one/two/c.pdf"_s }));
    }

    // --- Creating -----------------------------------------------------------

    void createFileNamesAreUnique()
    {
        Fixture_ f{};

        QCOMPARE(f.vault.createFile(f.rootKey()), f.key(u"Untitled.txt"_s));
        QCOMPARE(f.vault.createFile(f.rootKey()), f.key(u"Untitled 1.txt"_s));
        QCOMPARE(f.vault.createFile(f.rootKey()), f.key(u"Untitled 2.txt"_s));

        QVERIFY(f.exists(u"Untitled.txt"_s));
        QVERIFY(f.exists(u"Untitled 1.txt"_s));
        QVERIFY(f.exists(u"Untitled 2.txt"_s));
        QCOMPARE(f.read(u"Untitled.txt"_s), QByteArray(""));
    }

    void createFolderNamesAreUnique()
    {
        Fixture_ f{};

        QCOMPARE(f.vault.createFolder(f.rootKey()), f.key(u"Untitled"_s));
        QCOMPARE(f.vault.createFolder(f.rootKey()), f.key(u"Untitled 1"_s));

        QVERIFY(QFileInfo(f.folder.filePath(u"Untitled"_s)).isDir());
        QVERIFY(QFileInfo(f.folder.filePath(u"Untitled 1"_s)).isDir());
    }

    void createInsideAFolder()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QCOMPARE(
            f.vault.createFile(f.key(u"one"_s)),
            f.key(u"one/Untitled.txt"_s));
        QCOMPARE(
            f.vault.createFolder(f.key(u"one"_s)),
            f.key(u"one/Untitled"_s));
    }

    // A new file can be opened at once
    void createdFileOpens()
    {
        Fixture_ f{};
        auto key = f.vault.createFile(f.rootKey());

        auto* model = f.open(key);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray(""));
    }

    // A folder that has gone stays gone
    void createInAMissingFolderFailsAndMakesNothing()
    {
        Fixture_ f{};

        QVERIFY(f.vault.createFile(f.key(u"gone"_s)).isEmpty());
        QVERIFY(f.vault.createFolder(f.key(u"gone"_s)).isEmpty());
        QVERIFY(!f.exists(u"gone"_s));
    }

    void createFileAtMakesTheFoldersAboveIt()
    {
        Fixture_ f{};

        QVERIFY(
            f.vault.createFileAt(f.key(u"drafts/part one/Chapter 2.txt"_s)));

        QVERIFY(QFileInfo(f.folder.filePath(u"drafts/part one"_s)).isDir());
        QCOMPARE(f.read(u"drafts/part one/Chapter 2.txt"_s), QByteArray(""));
        QVERIFY(f.open(u"drafts/part one/Chapter 2.txt"_s));
    }

    void createFileAtInAFolderThatIsThere()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QVERIFY(f.vault.createFileAt(f.key(u"one/Notes.md"_s)));
        QVERIFY(f.vault.createFileAt(f.key(u"Script.fountain"_s)));

        QVERIFY(f.exists(u"one/Notes.md"_s));
        QVERIFY(f.exists(u"Script.fountain"_s));
    }

    // The file is left as it was
    void createFileAtAnExistingFileIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "kept");

        QVERIFY(!f.vault.createFileAt(f.key(u"a.txt"_s)));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("kept"));
    }

    // A part of the path is a file, so there is no folder to make
    void createFileAtThroughAFileFails()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "kept");

        QVERIFY(!f.vault.createFileAt(f.key(u"a.txt/b.txt"_s)));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("kept"));
    }

    void createFileAtRefusesWhatIsNotAFileInside()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;

        QVERIFY(!f.vault.createFileAt(PathKey{}));
        QVERIFY(!f.vault.createFileAt(f.key(u"../"_s + escapee)));
        QVERIFY(!f.vault.createFileAt(f.rootKey()));

        QVERIFY(!QFileInfo(QFileInfo(f.folder.path()).dir().filePath(escapee))
                     .exists());
    }

    // A key is normalized, so a path through "." or ".." names the file it
    // ends at, and only that file's folders are made
    void createFileAtTakesAnySpelling()
    {
        Fixture_ f{};

        QVERIFY(f.vault.createFileAt(f.key(u"a/../b.txt"_s)));
        QVERIFY(f.vault.createFileAt(f.key(u"c/./d.txt"_s)));

        QVERIFY(f.exists(u"b.txt"_s));
        QVERIFY(f.exists(u"c/d.txt"_s));
        QVERIFY(!f.exists(u"a"_s));
    }

    void createFileAtRefusesHiddenNamesAndOtherTypes()
    {
        Fixture_ f{};

        QVERIFY(!f.vault.createFileAt(f.key(u".hidden.txt"_s)));
        QVERIFY(!f.vault.createFileAt(f.key(u".notes/a.txt"_s)));
        QVERIFY(!f.vault.createFileAt(f.key(u"one/.notes/a.txt"_s)));
        QVERIFY(!f.vault.createFileAt(f.key(u"scan.pdf"_s)));
        QVERIFY(!f.vault.createFileAt(f.key(u"picture.png"_s)));
        QVERIFY(!f.vault.createFileAt(f.key(u"Chapter 2"_s)));

        QVERIFY(!f.exists(u".hidden.txt"_s));
        QVERIFY(!f.exists(u".notes"_s));
        QVERIFY(!f.exists(u"one"_s));
        QVERIFY(!f.exists(u"scan.pdf"_s));
        QVERIFY(!f.exists(u"picture.png"_s));
        QVERIFY(!f.exists(u"Chapter 2"_s));
    }

    // --- Renaming -----------------------------------------------------------

    void renameAFile()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");

        auto renamed = f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s);

        QCOMPARE(renamed, f.key(u"b.txt"_s));
        QVERIFY(!f.exists(u"a.txt"_s));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("hello"));
    }

    // The buffer follows its file: same buffer, new identity
    void renameAnOpenFile()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy renamed(model, &AbstractFileModel::renamed);

        f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s);

        QCOMPARE(renamed.count(), 1);
        QCOMPARE(model->key(), f.key(u"b.txt"_s));
        QCOMPARE(model->title(), u"b"_s);
        QCOMPARE(f.open(u"b.txt"_s), model);
        QVERIFY(!f.open(u"a.txt"_s));
    }

    // An edit made after a rename is saved under the new name, and the old
    // name isn't written back
    void renamedOpenFileSavesToItsNewName()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s);
        type_(model, u"!"_s);

        QVERIFY(f.store.flush().isEmpty());
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("hello!"));
        QVERIFY(!f.exists(u"a.txt"_s));
    }

    // Unsaved typing goes with the file: a rename moves the file as it is on
    // disk, and the buffer still holds the typing to save
    void renameKeepsUnsavedTyping()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);
        type_(model, u"!"_s);

        f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s);

        QVERIFY(model->isModified());
        QCOMPARE(model->data(), QByteArray("hello!"));

        QVERIFY(f.store.flush().isEmpty());
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("hello!"));
    }

    // Every open buffer under a renamed folder follows it
    void renameAFolderWithOpenFilesInside()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        f.write(u"one/two/b.txt"_s, "b");
        f.write(u"other.txt"_s, "c");

        auto* a = f.open(u"one/a.txt"_s);
        auto* b = f.open(u"one/two/b.txt"_s);
        auto* other = f.open(u"other.txt"_s);
        QVERIFY(a && b && other);

        auto renamed = f.vault.rename(f.key(u"one"_s), u"uno"_s);

        QCOMPARE(renamed, f.key(u"uno"_s));
        QCOMPARE(a->key(), f.key(u"uno/a.txt"_s));
        QCOMPARE(b->key(), f.key(u"uno/two/b.txt"_s));
        QCOMPARE(other->key(), f.key(u"other.txt"_s));

        QCOMPARE(f.open(u"uno/a.txt"_s), a);
        QCOMPARE(f.open(u"uno/two/b.txt"_s), b);
        QVERIFY(f.exists(u"uno/two/b.txt"_s));
        QVERIFY(!f.exists(u"one"_s));
    }

    // A folder whose name only starts like the renamed one is left alone
    void renameAFolderLeavesItsNamesakeAlone()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        f.write(u"one more/b.txt"_s, "b");

        auto* b = f.open(u"one more/b.txt"_s);
        QVERIFY(b);

        f.vault.rename(f.key(u"one"_s), u"uno"_s);

        QCOMPARE(b->key(), f.key(u"one more/b.txt"_s));
        QVERIFY(f.exists(u"one more/b.txt"_s));
    }

    void renameChangingOnlyLetterCase()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");
        auto* model = f.open(u"one/a.txt"_s);
        QVERIFY(model);

        auto renamed = f.vault.rename(f.key(u"one/a.txt"_s), u"A.TXT"_s);

        QCOMPARE(renamed, f.key(u"one/A.TXT"_s));
        QCOMPARE(f.namesIn(u"one"_s), QStringList({ u"A.TXT"_s }));
        QCOMPARE(f.read(u"one/A.TXT"_s), QByteArray("hello"));
        QCOMPARE(model->key(), f.key(u"one/A.TXT"_s));
    }

    void renameOntoATakenNameFailsAndChangesNothing()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QVERIFY(f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s).isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("b"));
        QCOMPARE(model->key(), f.key(u"a.txt"_s));
        QCOMPARE(f.open(u"a.txt"_s), model);
    }

    // A file deleted outside Fernanda keeps its buffer until the watcher's
    // report is reconciled. A rename onto its name in the meantime discards
    // that buffer, as the reconcile would, and the renamed file's buffer takes
    // the name
    void renameOntoANameJustDeletedOutsideReplacesItsBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        auto* a = f.open(u"a.txt"_s);
        auto* b = f.open(u"b.txt"_s);
        QVERIFY(a && b);

        QSignalSpy removed(b, &AbstractFileModel::removedFromDisk);
        QVERIFY(QFile::remove(f.folder.filePath(u"b.txt"_s)));

        QCOMPARE(
            f.vault.rename(f.key(u"a.txt"_s), u"b.txt"_s),
            f.key(u"b.txt"_s));

        QCOMPARE(removed.count(), 1);
        QCOMPARE(a->key(), f.key(u"b.txt"_s));
        QCOMPARE(f.open(u"b.txt"_s), a);
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("a"));
    }

    void renameAMissingEntryFails()
    {
        Fixture_ f{};

        QVERIFY(f.vault.rename(f.key(u"gone.txt"_s), u"b.txt"_s).isEmpty());
        QVERIFY(!f.exists(u"b.txt"_s));
    }

    // --- Paths, however they're spelled ------------------------------------

    // A key is normalized, so "." and ".." name where they end up: inside the
    // vault, the same file as the plain path; outside it, refused like any
    // other path outside.
    //
    // Where a test tries to put a file outside the vault, the file's name is
    // built from the temporary folder's own name, so if it does get out it is
    // found and removed, and nothing else beside the folder can share its name

    void containsGoesByWhereAPathEnds()
    {
        Fixture_ f{};

        QVERIFY(f.vault.contains(f.key(u"one/.."_s)));
        QVERIFY(f.vault.contains(f.key(u"one/./a.txt"_s)));
        QVERIFY(f.vault.contains(f.key(u"one/../one/a.txt"_s)));
        QVERIFY(f.vault.contains(
            PathKey::of(Coco::Path(f.folder.path() + u"/"_s))));
        QVERIFY(!f.vault.contains(f.key(u"one/../.."_s)));
        QVERIFY(!f.vault.contains(f.key(u"../x.txt"_s)));
    }

    void createOutsideTheVaultIsRefused()
    {
        Fixture_ f{};
        QTemporaryDir elsewhere{};
        auto elsewhere_key = PathKey::of(Coco::Path(elsewhere.path()));

        QVERIFY(f.vault.createFile(elsewhere_key).isEmpty());
        QVERIFY(f.vault.createFolder(elsewhere_key).isEmpty());
        QVERIFY(QDir(elsewhere.path())
                    .entryList(QDir::AllEntries | QDir::NoDotAndDotDot)
                    .isEmpty());
    }

    // A folder inside the vault, by a path through "." or ".."
    void createInAFolderByAnySpelling()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QCOMPARE(
            f.vault.createFile(f.key(u"one/../one"_s)),
            f.key(u"one/Untitled.txt"_s));
        QCOMPARE(
            f.vault.createFolder(f.key(u"one/."_s)),
            f.key(u"one/Untitled"_s));
    }

    void renameCannotLeaveTheVault()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;
        auto outside = QFileInfo(f.folder.path()).dir().filePath(escapee);
        f.write(u"one/a.txt"_s, "hello");

        auto renamed =
            f.vault.rename(f.key(u"one/a.txt"_s), u"../../"_s + escapee);

        auto left = QFileInfo::exists(outside);
        QFile::remove(outside);

        QVERIFY(renamed.isEmpty());
        QVERIFY(!left);
        QVERIFY(f.exists(u"one/a.txt"_s));
    }

    void renameTakesOnlyASingleName()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");
        f.makeFolder(u"two"_s);
        auto* model = f.open(u"one/a.txt"_s);
        QVERIFY(model);

        auto old = f.key(u"one/a.txt"_s);

        QVERIFY(f.vault.rename(old, u"../moved.txt"_s).isEmpty());
        QVERIFY(f.vault.rename(old, u"../two/moved.txt"_s).isEmpty());
        QVERIFY(f.vault.rename(old, u"sub/moved.txt"_s).isEmpty());
        QVERIFY(f.vault.rename(old, u".."_s).isEmpty());
        QVERIFY(f.vault.rename(old, u"."_s).isEmpty());
        QVERIFY(f.vault.rename(old, u"moved.txt/"_s).isEmpty());
        QVERIFY(f.vault.rename(old, QString{}).isEmpty());

        QVERIFY(f.exists(u"one/a.txt"_s));
        QVERIFY(!f.exists(u"moved.txt"_s));
        QVERIFY(f.namesIn(u"two"_s).isEmpty());
        QCOMPARE(model->key(), f.key(u"one/a.txt"_s));
    }

    void moveCannotLeaveTheVault()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;
        auto outside = QFileInfo(f.folder.path()).dir().filePath(escapee);
        f.write(u"one/"_s + escapee, "hello");

        // The folder above the vault, written as a path under the vault
        auto moved =
            f.vault.move(f.key(u"one/"_s + escapee), f.key(u"one/../.."_s));

        auto left = QFileInfo::exists(outside);
        QFile::remove(outside);

        QVERIFY(moved.isEmpty());
        QVERIFY(!left);
        QVERIFY(f.exists(u"one/"_s + escapee));
    }

    void moveTakesAnySpelling()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        f.makeFolder(u"one"_s);
        f.makeFolder(u"two"_s);

        QCOMPARE(
            f.vault.move(f.key(u"two/../a.txt"_s), f.key(u"one/../two/."_s)),
            f.key(u"two/a.txt"_s));
        QVERIFY(f.exists(u"two/a.txt"_s));
    }

    // --- Moving -------------------------------------------------------------

    void moveAFileIntoAFolder()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        f.makeFolder(u"one"_s);
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        auto moved = f.vault.move(f.key(u"a.txt"_s), f.key(u"one"_s));

        QCOMPARE(moved, f.key(u"one/a.txt"_s));
        QVERIFY(!f.exists(u"a.txt"_s));
        QCOMPARE(f.read(u"one/a.txt"_s), QByteArray("hello"));
        QCOMPARE(model->key(), f.key(u"one/a.txt"_s));
        QCOMPARE(f.open(u"one/a.txt"_s), model);
    }

    void moveAFileToTheRoot()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");

        auto moved = f.vault.move(f.key(u"one/a.txt"_s), f.rootKey());

        QCOMPARE(moved, f.key(u"a.txt"_s));
        QVERIFY(f.exists(u"a.txt"_s));
    }

    void moveAFolderWithOpenFilesInside()
    {
        Fixture_ f{};
        f.write(u"one/two/a.txt"_s, "hello");
        f.makeFolder(u"other"_s);
        auto* model = f.open(u"one/two/a.txt"_s);
        QVERIFY(model);

        auto moved = f.vault.move(f.key(u"one"_s), f.key(u"other"_s));

        QCOMPARE(moved, f.key(u"other/one"_s));
        QCOMPARE(model->key(), f.key(u"other/one/two/a.txt"_s));
        QVERIFY(f.exists(u"other/one/two/a.txt"_s));
        QVERIFY(!f.exists(u"one"_s));
    }

    void moveAFolderIntoItselfIsRefused()
    {
        Fixture_ f{};
        f.write(u"one/two/a.txt"_s, "hello");

        QVERIFY(f.vault.move(f.key(u"one"_s), f.key(u"one"_s)).isEmpty());
        QVERIFY(f.vault.move(f.key(u"one"_s), f.key(u"one/two"_s)).isEmpty());

        QVERIFY(f.exists(u"one/two/a.txt"_s));
    }

    void moveOntoATakenNameIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "top");
        f.write(u"one/a.txt"_s, "inner");

        QVERIFY(f.vault.move(f.key(u"a.txt"_s), f.key(u"one"_s)).isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("top"));
        QCOMPARE(f.read(u"one/a.txt"_s), QByteArray("inner"));
    }

    void moveIntoSomethingThatIsNotAFolderIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");

        QVERIFY(f.vault.move(f.key(u"a.txt"_s), f.key(u"b.txt"_s)).isEmpty());
        QVERIFY(f.vault.move(f.key(u"a.txt"_s), f.key(u"gone"_s)).isEmpty());

        QVERIFY(f.exists(u"a.txt"_s));
    }

    void moveAcrossTheVaultsEdgeIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "inside");

        QTemporaryDir elsewhere{};
        QFile outside(elsewhere.filePath(u"b.txt"_s));
        QVERIFY(outside.open(QIODevice::WriteOnly));
        outside.write("outside");
        outside.close();

        // Out of the vault
        QVERIFY(f.vault
                    .move(
                        f.key(u"a.txt"_s),
                        PathKey::of(Coco::Path(elsewhere.path())))
                    .isEmpty());

        // Into the vault
        QVERIFY(f.vault
                    .move(
                        PathKey::of(Coco::Path(elsewhere.filePath(u"b.txt"_s))),
                        f.rootKey())
                    .isEmpty());

        QVERIFY(f.exists(u"a.txt"_s));
        QVERIFY(!f.exists(u"b.txt"_s));
        QVERIFY(QFileInfo::exists(elsewhere.filePath(u"b.txt"_s)));
        QVERIFY(!QFileInfo::exists(elsewhere.filePath(u"a.txt"_s)));
    }

    // --- Settings -----------------------------------------------------------

    void aSettingIsAnnouncedOnceAndSavedOnRequest()
    {
        QTemporaryDir folder{};
        Coco::Path root(folder.path());
        BufferStore store{ nullptr };

        {
            Vault vault(root, &store, nullptr);
            QSignalSpy changed(&vault, &Vault::configChanged);

            auto size = vault.config().textFontSize() + 3;
            vault.setConfig(&VaultConfig::setTextFontSize, size);
            vault.setConfig(
                &VaultConfig::setWrapLines,
                !vault.config().wrapLines());

            QCOMPARE(changed.count(), 2);

            // Setting what is already set is no change
            vault.setConfig(&VaultConfig::setTextFontSize, size);
            QCOMPARE(changed.count(), 2);

            vault.saveSettingsNow();
        }

        // A second vault on the same folder reads what the first saved
        Vault reopened(root, &store, nullptr);

        QCOMPARE(
            reopened.config().textFontSize(),
            VaultConfig::DEFAULT_TEXT_FONT_SIZE + 3);
        QCOMPARE(
            reopened.config().wrapLines(),
            !VaultConfig::DEFAULT_WRAP_LINES);
    }

    // --- Spelling words -----------------------------------------------------

    // The dictionary file is read when the vault opens
    void theDictionaryIsReadAtConstruction()
    {
        QTemporaryDir folder{};
        Coco::Path root(folder.path());
        BufferStore store{ nullptr };

        {
            Vault first(root, &store, nullptr);
        }

        QFile file(folder.filePath(u".fernanda/dictionary.txt"_s));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Pangloss\nCunegonde\n");
        file.close();

        Vault vault(root, &store, nullptr);

        QVERIFY(vault.dictionary().contains(u"Pangloss"_s));
        QVERIFY(vault.dictionary().contains(u"Cunegonde"_s));
        QCOMPARE(vault.dictionary().size(), 2);
    }

    // Added, announced, and written sorted, one word per line
    void anAddedWordIsAnnouncedAndSaved()
    {
        Fixture_ f{};
        QSignalSpy changed(&f.vault, &Vault::wordsChanged);

        f.vault.addToDictionary(u"Pangloss"_s);
        f.vault.addToDictionary(u"Cunegonde"_s);

        QCOMPARE(changed.count(), 2);
        QVERIFY(f.vault.dictionary().contains(u"Pangloss"_s));
        QCOMPARE(
            f.read(u".fernanda/dictionary.txt"_s),
            QByteArray("Cunegonde\nPangloss\n"));

        // A word already there is no change
        f.vault.addToDictionary(u"Pangloss"_s);
        QCOMPARE(changed.count(), 2);
    }

    // Adding reads the file again, so what was added or removed by hand since
    // the vault opened stays as the file has it
    void addingKeepsTheFileAsEditedByHand()
    {
        Fixture_ f{};
        f.vault.addToDictionary(u"Pangloss"_s);

        f.write(u".fernanda/dictionary.txt"_s, "Candide\n");
        f.vault.addToDictionary(u"Cunegonde"_s);

        QCOMPARE(
            f.read(u".fernanda/dictionary.txt"_s),
            QByteArray("Candide\nCunegonde\n"));
        QVERIFY(f.vault.dictionary().contains(u"Candide"_s));
        QVERIFY(!f.vault.dictionary().contains(u"Pangloss"_s));
    }

    // Ignored words are announced, kept apart from the dictionary, and not
    // written anywhere
    void anIgnoredWordIsAnnouncedButNotSaved()
    {
        Fixture_ f{};
        QSignalSpy changed(&f.vault, &Vault::wordsChanged);

        f.vault.ignoreWord(u"Thunder"_s);
        f.vault.ignoreWord(u"Thunder"_s);

        QCOMPARE(changed.count(), 1);
        QVERIFY(f.vault.ignoredWords().contains(u"Thunder"_s));
        QVERIFY(!f.vault.dictionary().contains(u"Thunder"_s));
        QVERIFY(!f.exists(u".fernanda/dictionary.txt"_s));
    }

    // A vault folder made again after it vanished holds no settings and no
    // words, and the vault takes that on, announcing both so every window
    // applies it. The words ignored in the old folder go too
    void recreateRootStartsFromTheNewFolder()
    {
        Fixture_ f{};
        auto default_size = f.vault.config().textFontSize();

        f.vault.setConfig(&VaultConfig::setTextFontSize, default_size + 3);
        f.vault.saveSettingsNow();
        f.vault.addToDictionary(u"Pangloss"_s);
        f.vault.ignoreWord(u"Thunder"_s);

        QVERIFY(QDir(f.folder.path()).removeRecursively());

        QSignalSpy config_changed(&f.vault, &Vault::configChanged);
        QSignalSpy words_changed(&f.vault, &Vault::wordsChanged);

        QVERIFY(f.vault.recreateRoot(f.rootKey()));

        QVERIFY(f.root.isDir());
        QCOMPARE(config_changed.count(), 1);
        QCOMPARE(words_changed.count(), 1);
        QCOMPARE(f.vault.config().textFontSize(), default_size);
        QVERIFY(f.vault.dictionary().isEmpty());
        QVERIFY(f.vault.ignoredWords().isEmpty());
    }

    // A key for another folder is refused, and nothing is made
    void recreateRootRefusesAnotherFolder()
    {
        Fixture_ f{};
        QVERIFY(QDir(f.folder.path()).removeRecursively());

        QVERIFY(!f.vault.recreateRoot(f.key(u"sub"_s)));
        QVERIFY(!f.vault.recreateRoot(PathKey{}));

        QVERIFY(!f.root.exists());
        QVERIFY(!f.absolute(u"sub"_s).exists());
    }

    // Where letter case is ignored, the folder can be made again in another
    // case: on disk, as the root, and in the tree, announced so the windows'
    // copies of the root follow
    void recreateRootMakesTheFolderInTheGivenSpelling()
    {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        Fixture_ f{};
        auto upper_name = f.root.nameQString().toUpper();
        auto upper = f.root.parent() / Coco::Path(upper_name);

        if (!upper.isDir()) {
            QSKIP("This file system respects letter case");
        }

        QVERIFY(QDir(f.folder.path()).removeRecursively());
        QSignalSpy respelled(&f.vault, &Vault::rootRespelled);

        QVERIFY(f.vault.recreateRoot(PathKey::of(upper)));

        QCOMPARE(respelled.count(), 1);
        QCOMPARE(f.vault.root().nameQString(), upper_name);
        QCOMPARE(f.vault.treeModel()->root(), f.vault.root());
        auto names = QDir(f.root.parent().toQString())
                         .entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QVERIFY(names.contains(upper_name));

#else

        QSKIP("Letter case counts in paths on this platform");

#endif
    }

    // --- Vaults inside vaults -----------------------------------------------

    // Each test makes an inner vault in a folder of the fixture's, lent the
    // same store, as App lends its one store to every vault. The inner vault
    // is made after the fixture, so it's destroyed first

    void aFileInsideTwoVaultsIsOneBuffer()
    {
        Fixture_ f{};
        f.write(u"inner/notes.txt"_s, "hello");
        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);

        auto key = f.key(u"inner/notes.txt"_s);
        auto* model = f.open(key);

        QVERIFY(model);
        QVERIFY(f.vault.contains(key));
        QVERIFY(inner.contains(key));
        QCOMPARE(
            f.open(PathKey::of(inner.root() / Coco::Path(u"notes.txt"_s))),
            model);
    }

    // The buffer follows a rename made through the outer vault, typing and
    // all, and is saved where the file now is
    void aRenameThroughTheOuterVaultCarriesTheBuffer()
    {
        Fixture_ f{};
        f.write(u"inner/sub/notes.txt"_s, "hello");
        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);

        auto* model = f.open(u"inner/sub/notes.txt"_s);
        QVERIFY(model);
        type_(model, u"!"_s);

        QCOMPARE(
            f.vault.rename(f.key(u"inner/sub"_s), u"renamed"_s),
            f.key(u"inner/renamed"_s));

        QCOMPARE(model->key(), f.key(u"inner/renamed/notes.txt"_s));
        QVERIFY(inner.contains(model->key()));
        QVERIFY(f.store.flush().isEmpty());
        QCOMPARE(f.read(u"inner/renamed/notes.txt"_s), QByteArray("hello!"));
        QVERIFY(!f.exists(u"inner/sub"_s));
    }

    // The same buffer, so the outer vault finds it under the new name
    void aRenameThroughTheInnerVaultIsSeenByTheOuter()
    {
        Fixture_ f{};
        f.write(u"inner/notes.txt"_s, "hello");
        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);

        auto* model = f.open(u"inner/notes.txt"_s);
        QVERIFY(model);

        inner.rename(
            PathKey::of(inner.root() / Coco::Path(u"notes.txt"_s)),
            u"renamed.txt"_s);

        QCOMPARE(f.open(u"inner/renamed.txt"_s), model);
    }

    // Moved out of the inner vault, the file is no longer the inner vault's,
    // and its buffer goes with it
    void aMoveOutOfTheInnerVaultCarriesTheBuffer()
    {
        Fixture_ f{};
        f.write(u"inner/notes.txt"_s, "hello");
        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);

        auto* model = f.open(u"inner/notes.txt"_s);
        QVERIFY(model);

        QCOMPARE(
            f.vault.move(f.key(u"inner/notes.txt"_s), f.rootKey()),
            f.key(u"notes.txt"_s));

        QCOMPARE(model->key(), f.key(u"notes.txt"_s));
        QVERIFY(!inner.contains(model->key()));
        QCOMPARE(f.read(u"notes.txt"_s), QByteArray("hello"));
    }

    // Typing not yet saved, in a file shown only in the inner vault's window,
    // survives the outer vault moving the file out of the inner vault. The
    // buffer follows the file, and when the inner window closes its view, the
    // last view's save writes the text to the file's new place
    void unsavedTypingFollowsAMoveOutOfTheInnerVault()
    {
        Fixture_ f{};
        f.write(u"inner/notes.txt"_s, "hello");
        Vault inner(f.absolute(u"inner"_s), &f.store, nullptr);

        auto* model = f.open(u"inner/notes.txt"_s);
        QVERIFY(model);

        {
            QObject inner_view{};
            model->addView(&inner_view);
            type_(model, u"!"_s);

            QCOMPARE(
                f.vault.move(f.key(u"inner/notes.txt"_s), f.rootKey()),
                f.key(u"notes.txt"_s));
            QVERIFY(!inner.contains(model->key()));
        }

        QCOMPARE(f.read(u"notes.txt"_s), QByteArray("hello!"));
        QVERIFY(!f.exists(u"inner/notes.txt"_s));
    }

    // Where letter case is ignored, the outer vault renaming a folder above
    // the inner vault's by letter case alone leaves the inner vault in place.
    // Respelled, as App does then, the inner vault's root, its tree, and the
    // files it lists all take the new spelling
    void respellRootTakesTheNewLetterCase()
    {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        Fixture_ f{};
        f.write(u"box/inner/sub/a.txt"_s, "a");
        Vault inner(f.absolute(u"box/inner"_s), &f.store, nullptr);

        if (!f.absolute(u"BOX"_s).isDir()) {
            QSKIP("This file system respects letter case");
        }

        QCOMPARE(f.vault.rename(f.key(u"box"_s), u"BOX"_s), f.key(u"BOX"_s));

        QSignalSpy respelled(&inner, &Vault::rootRespelled);
        inner.respellRoot(
            inner.rootKey().rebased(f.key(u"box"_s), f.key(u"BOX"_s)));

        QCOMPARE(respelled.count(), 1);
        QCOMPARE(inner.root().parent().nameQString(), u"BOX"_s);
        QCOMPARE(inner.rootKey().path(), inner.root());
        QCOMPARE(inner.treeModel()->root(), inner.root());
        QVERIFY(inner.contains(f.key(u"box/inner/sub/a.txt"_s)));

        auto files = inner.visibleFiles();
        QCOMPARE(files.size(), 1);
        QVERIFY(files.first().path().isUnder(inner.root()));

#else

        QSKIP("Letter case counts in paths on this platform");

#endif
    }

    // --- Trash refusals -----------------------------------------------------

    // Each of these is refused before the system trash is asked for anything
    void moveToTrashRefusals()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        f.makeFolder(u"one"_s);

        QTemporaryDir elsewhere{};
        QFile outside(elsewhere.filePath(u"b.txt"_s));
        QVERIFY(outside.open(QIODevice::WriteOnly));
        outside.close();

        QVERIFY(!f.vault.moveToTrash(
            PathKey::of(Coco::Path(elsewhere.filePath(u"b.txt"_s)))));
        QVERIFY(!f.vault.moveToTrash(f.rootKey()));
        QVERIFY(!f.vault.moveToTrash(f.key(u"gone.txt"_s)));
        QVERIFY(!f.vault.moveToTrash(f.key(u"../x.txt"_s)));

        // The root, by other spellings
        QVERIFY(!f.vault.moveToTrash(
            PathKey::of(Coco::Path(f.folder.path() + u"/"_s))));
        QVERIFY(!f.vault.moveToTrash(
            PathKey::of(Coco::Path(f.folder.path() + u"/."_s))));
        QVERIFY(!f.vault.moveToTrash(f.key(u"one/.."_s)));

        QVERIFY(QFileInfo::exists(elsewhere.filePath(u"b.txt"_s)));
        QVERIFY(QFileInfo(f.folder.path()).isDir());
        QVERIFY(f.exists(u"a.txt"_s));
    }
};

QTEST_MAIN(VaultTest)

#include "VaultTest.moc"
