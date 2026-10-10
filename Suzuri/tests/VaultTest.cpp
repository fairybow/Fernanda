/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QImage>
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

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "models/AbstractFileModel.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using Suzuri::AbstractFileModel;
using Suzuri::TextFileModel;
using Suzuri::Vault;
using Suzuri::VaultConfig;

// What a Vault does with files and their buffers (core/Vault.h), for
// everything that happens at once: opening, creating, renaming, moving,
// flushing, and freeing. What waits on a timer or the file watcher (autosave,
// external changes) is not covered here.
//
// Each test gets its own vault in a temporary folder.
//
// Qt Test runs every private slot as a test
class VaultTest : public QObject
{
    Q_OBJECT

private:
    // A vault in a temporary folder, and the file work a test does around it.
    // Paths given to these functions are relative to the vault's root.
    //
    // The vault is declared after the folder so it is destroyed first
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };
        Vault vault{ root, nullptr };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

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
        // vault asks one
        [[nodiscard]] AbstractFileModel* open(const QString& relative)
        {
            auto asked = false;

            auto* model = vault.openModel(
                Coco::Path(relative),
                [&asked](const Coco::Path&, const QByteArray&) {
                    asked = true;
                    return false;
                });

            if (asked) {
                QTest::qFail(
                    "The vault asked about a lossy open",
                    __FILE__,
                    __LINE__);
            }

            return model;
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

    [[nodiscard]] static QByteArray pngBytes_()
    {
        QImage image(2, 2, QImage::Format_RGB32);
        image.fill(Qt::red);

        QByteArray bytes{};
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");

        return bytes;
    }

private slots:
    // --- Paths --------------------------------------------------------------

    void containsIsByFolderNotByPrefix()
    {
        Fixture_ f{};

        QVERIFY(f.vault.contains(f.root));
        QVERIFY(f.vault.contains(f.absolute(u"a.txt"_s)));
        QVERIFY(f.vault.contains(f.absolute(u"one/two/a.txt"_s)));

        // A sibling folder whose name starts with the vault's name
        QVERIFY(!f.vault.contains(Coco::Path(f.folder.path() + u"x"_s)));
        QVERIFY(!f.vault.contains(f.root.parent()));
    }

    void relativeAndAbsoluteAreInverses()
    {
        Fixture_ f{};
        auto absolute = f.absolute(u"one/a.txt"_s);

        QCOMPARE(f.vault.relativePathOf(absolute), Coco::Path("one/a.txt"));
        QCOMPARE(f.vault.absolutePathOf(Coco::Path("one/a.txt")), absolute);

        auto ref = f.vault.makeFileRef(absolute);
        QCOMPARE(ref.vault, &f.vault);
        QCOMPARE(ref.relative, Coco::Path("one/a.txt"));
    }

    // The vault's own folder is made with the vault
    void constructionMakesTheDotFolder()
    {
        Fixture_ f{};

        QVERIFY(QFileInfo(f.folder.filePath(u".suzuri"_s)).isDir());
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

        for (const auto& path : f.vault.visibleFiles()) {
            listed << path.prettyQString();
        }

        listed.sort();

        QCOMPARE(
            listed,
            QStringList({ u"a.txt"_s, u"one/b.md"_s, u"one/two/c.pdf"_s }));
    }

    // --- Opening ------------------------------------------------------------

    void openGivesABufferHoldingTheFile()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello\n");

        auto* model = f.open(u"one/a.txt"_s);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray("hello\n"));
        QCOMPARE(model->fileRef().vault, &f.vault);
        QCOMPARE(model->fileRef().relative, Coco::Path("one/a.txt"));
        QCOMPARE(model->title(), u"a"_s);
        QVERIFY(!model->isModified());
    }

    void openTheSameFileTwiceGivesOneBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello\n");

        auto* first = f.open(u"a.txt"_s);
        auto* second = f.open(u"a.txt"_s);

        QVERIFY(first);
        QCOMPARE(first, second);
    }

    void openGivesEachTypeItsOwnKindOfBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "text");
        f.write(u"a.md"_s, "text");
        f.write(u"a.fountain"_s, "text");
        f.write(u"a.png"_s, pngBytes_());
        f.write(u"a.pdf"_s, "%PDF-1.4\n");

        QVERIFY(qobject_cast<TextFileModel*>(f.open(u"a.txt"_s)));
        QVERIFY(qobject_cast<TextFileModel*>(f.open(u"a.md"_s)));
        QVERIFY(qobject_cast<TextFileModel*>(f.open(u"a.fountain"_s)));

        auto* image = f.open(u"a.png"_s);
        QVERIFY(qobject_cast<Suzuri::ImageFileModel*>(image));
        QVERIFY(!image->isUserEditable());
        QCOMPARE(image->data(), pngBytes_());

        auto* pdf = f.open(u"a.pdf"_s);
        QVERIFY(qobject_cast<Suzuri::PdfFileModel*>(pdf));
        QVERIFY(!pdf->isUserEditable());
        QCOMPARE(pdf->data(), QByteArray("%PDF-1.4\n"));
    }

    void openRefusesAnUnsupportedType()
    {
        Fixture_ f{};
        f.write(u"a.docx"_s, "text");
        f.write(u"Makefile"_s, "text");

        QVERIFY(!f.open(u"a.docx"_s));
        QVERIFY(!f.open(u"Makefile"_s));
    }

    // A buffer over nothing would be written into existence by the first
    // keystroke
    void openRefusesAMissingFile()
    {
        Fixture_ f{};

        QVERIFY(!f.open(u"gone.txt"_s));
        QVERIFY(!f.exists(u"gone.txt"_s));
    }

    void openAnEmptyFile()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "");

        auto* model = f.open(u"a.txt"_s);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray(""));
    }

    // --- Opening text that isn't valid UTF-8 --------------------------------

    void openAsksAboutInvalidUtf8AndRefusesOnNo()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "caf\xE9");

        auto asked = 0;
        Coco::Path asked_about{};
        QByteArray asked_with{};

        auto* model = f.vault.openModel(
            Coco::Path("a.txt"),
            [&](const Coco::Path& relative, const QByteArray& data) {
                ++asked;
                asked_about = relative;
                asked_with = data;
                return false;
            });

        QVERIFY(!model);
        QCOMPARE(asked, 1);
        QCOMPARE(asked_about, Coco::Path("a.txt"));
        QCOMPARE(asked_with, QByteArray("caf\xE9"));

        // Declined, so the file is as it was
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("caf\xE9"));
    }

    // An accepted file is written back as valid UTF-8 straight away, so it
    // never needs asking about again
    void openRewritesInvalidUtf8OnYes()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "caf\xE9");

        auto asked = 0;
        auto accept = [&asked](const Coco::Path&, const QByteArray&) {
            ++asked;
            return true;
        };

        auto* model = f.vault.openModel(Coco::Path("a.txt"), accept);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray("caf\xEF\xBF\xBD"));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("caf\xEF\xBF\xBD"));
        QVERIFY(!model->isModified());

        // The open buffer answers a second open, and nothing is asked
        QCOMPARE(f.vault.openModel(Coco::Path("a.txt"), accept), model);
        QCOMPARE(asked, 1);
    }

    // Only text is asked about: other types are never decoded
    void openNeverAsksAboutAnImage()
    {
        Fixture_ f{};
        f.write(u"a.png"_s, "\xFF\xFE not an image");

        QVERIFY(f.open(u"a.png"_s));
    }

    // --- Creating -----------------------------------------------------------

    void createFileNamesAreUnique()
    {
        Fixture_ f{};

        QCOMPARE(f.vault.createFile(f.root), Coco::Path("Untitled.txt"));
        QCOMPARE(f.vault.createFile(f.root), Coco::Path("Untitled 1.txt"));
        QCOMPARE(f.vault.createFile(f.root), Coco::Path("Untitled 2.txt"));

        QVERIFY(f.exists(u"Untitled.txt"_s));
        QVERIFY(f.exists(u"Untitled 1.txt"_s));
        QVERIFY(f.exists(u"Untitled 2.txt"_s));
        QCOMPARE(f.read(u"Untitled.txt"_s), QByteArray(""));
    }

    void createFolderNamesAreUnique()
    {
        Fixture_ f{};

        QCOMPARE(f.vault.createFolder(f.root), Coco::Path("Untitled"));
        QCOMPARE(f.vault.createFolder(f.root), Coco::Path("Untitled 1"));

        QVERIFY(QFileInfo(f.folder.filePath(u"Untitled"_s)).isDir());
        QVERIFY(QFileInfo(f.folder.filePath(u"Untitled 1"_s)).isDir());
    }

    void createInsideAFolder()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QCOMPARE(
            f.vault.createFile(f.absolute(u"one"_s)),
            Coco::Path("one/Untitled.txt"));
        QCOMPARE(
            f.vault.createFolder(f.absolute(u"one"_s)),
            Coco::Path("one/Untitled"));
    }

    // A new file can be opened at once
    void createdFileOpens()
    {
        Fixture_ f{};
        auto relative = f.vault.createFile(f.root);

        auto* model = f.open(relative.toQString());

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray(""));
    }

    // A folder that has gone stays gone
    void createInAMissingFolderFailsAndMakesNothing()
    {
        Fixture_ f{};

        QVERIFY(f.vault.createFile(f.absolute(u"gone"_s)).isEmpty());
        QVERIFY(f.vault.createFolder(f.absolute(u"gone"_s)).isEmpty());
        QVERIFY(!f.exists(u"gone"_s));
    }

    void createFileAtMakesTheFoldersAboveIt()
    {
        Fixture_ f{};

        QVERIFY(
            f.vault.createFileAt(Coco::Path("drafts/part one/Chapter 2.txt")));

        QVERIFY(QFileInfo(f.folder.filePath(u"drafts/part one"_s)).isDir());
        QCOMPARE(f.read(u"drafts/part one/Chapter 2.txt"_s), QByteArray(""));
        QVERIFY(f.open(u"drafts/part one/Chapter 2.txt"_s));
    }

    void createFileAtInAFolderThatIsThere()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QVERIFY(f.vault.createFileAt(Coco::Path("one/Notes.md")));
        QVERIFY(f.vault.createFileAt(Coco::Path("Script.fountain")));

        QVERIFY(f.exists(u"one/Notes.md"_s));
        QVERIFY(f.exists(u"Script.fountain"_s));
    }

    // The file is left as it was
    void createFileAtAnExistingFileIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "kept");

        QVERIFY(!f.vault.createFileAt(Coco::Path("a.txt")));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("kept"));
    }

    // A part of the path is a file, so there is no folder to make
    void createFileAtThroughAFileFails()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "kept");

        QVERIFY(!f.vault.createFileAt(Coco::Path("a.txt/b.txt")));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("kept"));
    }

    void createFileAtRefusesWhatIsNotAKey()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;

        QVERIFY(!f.vault.createFileAt(Coco::Path{}));
        QVERIFY(!f.vault.createFileAt(Coco::Path(u"../"_s + escapee)));
        QVERIFY(!f.vault.createFileAt(Coco::Path("a/../b.txt")));
        QVERIFY(!f.vault.createFileAt(Coco::Path("a/./b.txt")));
        QVERIFY(!f.vault.createFileAt(f.absolute(u"b.txt"_s)));

        QVERIFY(!QFileInfo(QFileInfo(f.folder.path()).dir().filePath(escapee))
                     .exists());
        QVERIFY(!f.exists(u"a"_s));
        QVERIFY(!f.exists(u"b.txt"_s));
    }

    void createFileAtRefusesHiddenNamesAndOtherTypes()
    {
        Fixture_ f{};

        QVERIFY(!f.vault.createFileAt(Coco::Path(".hidden.txt")));
        QVERIFY(!f.vault.createFileAt(Coco::Path(".notes/a.txt")));
        QVERIFY(!f.vault.createFileAt(Coco::Path("one/.notes/a.txt")));
        QVERIFY(!f.vault.createFileAt(Coco::Path("scan.pdf")));
        QVERIFY(!f.vault.createFileAt(Coco::Path("picture.png")));
        QVERIFY(!f.vault.createFileAt(Coco::Path("Chapter 2")));

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

        auto renamed = f.vault.rename(f.absolute(u"a.txt"_s), u"b.txt"_s);

        QCOMPARE(renamed, Coco::Path("b.txt"));
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

        f.vault.rename(f.absolute(u"a.txt"_s), u"b.txt"_s);

        QCOMPARE(renamed.count(), 1);
        QCOMPARE(model->fileRef().relative, Coco::Path("b.txt"));
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

        f.vault.rename(f.absolute(u"a.txt"_s), u"b.txt"_s);
        type_(model, u"!"_s);

        QVERIFY(f.vault.flush().isEmpty());
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

        f.vault.rename(f.absolute(u"a.txt"_s), u"b.txt"_s);

        QVERIFY(model->isModified());
        QCOMPARE(model->data(), QByteArray("hello!"));

        QVERIFY(f.vault.flush().isEmpty());
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

        auto renamed = f.vault.rename(f.absolute(u"one"_s), u"uno"_s);

        QCOMPARE(renamed, Coco::Path("uno"));
        QCOMPARE(a->fileRef().relative, Coco::Path("uno/a.txt"));
        QCOMPARE(b->fileRef().relative, Coco::Path("uno/two/b.txt"));
        QCOMPARE(other->fileRef().relative, Coco::Path("other.txt"));

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

        f.vault.rename(f.absolute(u"one"_s), u"uno"_s);

        QCOMPARE(b->fileRef().relative, Coco::Path("one more/b.txt"));
        QVERIFY(f.exists(u"one more/b.txt"_s));
    }

    void renameChangingOnlyLetterCase()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");
        auto* model = f.open(u"one/a.txt"_s);
        QVERIFY(model);

        auto renamed = f.vault.rename(f.absolute(u"one/a.txt"_s), u"A.TXT"_s);

        QCOMPARE(renamed, Coco::Path("one/A.TXT"));
        QCOMPARE(f.namesIn(u"one"_s), QStringList({ u"A.TXT"_s }));
        QCOMPARE(f.read(u"one/A.TXT"_s), QByteArray("hello"));
        QCOMPARE(model->fileRef().relative, Coco::Path("one/A.TXT"));
    }

    void renameOntoATakenNameFailsAndChangesNothing()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QVERIFY(f.vault.rename(f.absolute(u"a.txt"_s), u"b.txt"_s).isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("b"));
        QCOMPARE(model->fileRef().relative, Coco::Path("a.txt"));
        QCOMPARE(f.open(u"a.txt"_s), model);
    }

    void renameAMissingEntryFails()
    {
        Fixture_ f{};

        QVERIFY(
            f.vault.rename(f.absolute(u"gone.txt"_s), u"b.txt"_s).isEmpty());
        QVERIFY(!f.exists(u"b.txt"_s));
    }

    // --- Paths that are not plain -------------------------------------------

    // The vault takes a path only as a list of names: no "." or "..", no
    // trailing separator, and for a relative path no root. Anything else is
    // refused, whether or not it would land inside the vault.
    //
    // Where a test tries to put a file outside the vault, the file's name is
    // built from the temporary folder's own name, so if it does get out it is
    // found and removed, and nothing else beside the folder can share its name

    void containsRefusesAPathThatIsNotPlain()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QVERIFY(!f.vault.contains(f.absolute(u"one/.."_s)));
        QVERIFY(!f.vault.contains(f.absolute(u"one/../.."_s)));
        QVERIFY(!f.vault.contains(f.absolute(u"one/./a.txt"_s)));
        QVERIFY(!f.vault.contains(f.absolute(u"one/../one/a.txt"_s)));
        QVERIFY(!f.vault.contains(Coco::Path(f.folder.path() + u"/"_s)));
        QVERIFY(!f.vault.contains(Coco::Path(f.folder.path() + u"/."_s)));
    }

    void openRefusesAPathThroughDots()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;
        auto outside = QFileInfo(f.folder.path()).dir().filePath(escapee);
        f.write(u"one/a.txt"_s, "hello");

        QFile file(outside);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("outside");
        file.close();

        auto* out = f.open(u"../"_s + escapee);
        auto* around = f.open(u"one/../one/a.txt"_s);
        auto* dotted = f.open(u"./one/a.txt"_s);

        QFile::remove(outside);

        QVERIFY(!out);
        QVERIFY(!around);
        QVERIFY(!dotted);
    }

    void openRefusesAnAbsolutePath()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");

        QTemporaryDir elsewhere{};
        QFile outside(elsewhere.filePath(u"b.txt"_s));
        QVERIFY(outside.open(QIODevice::WriteOnly));
        outside.close();

        QVERIFY(!f.open(elsewhere.filePath(u"b.txt"_s)));
        QVERIFY(!f.open(f.absolute(u"a.txt"_s).toQString()));
        QVERIFY(!f.open(QString{}));
    }

    void createOutsideTheVaultIsRefused()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);
        QTemporaryDir elsewhere{};

        QVERIFY(f.vault.createFile(Coco::Path(elsewhere.path())).isEmpty());
        QVERIFY(f.vault.createFolder(Coco::Path(elsewhere.path())).isEmpty());
        QVERIFY(QDir(elsewhere.path())
                    .entryList(QDir::AllEntries | QDir::NoDotAndDotDot)
                    .isEmpty());

        // A folder inside the vault, by a path that is not plain
        QVERIFY(f.vault.createFile(f.absolute(u"one/../one"_s)).isEmpty());
        QVERIFY(f.vault.createFolder(f.absolute(u"one/."_s)).isEmpty());
        QVERIFY(f.namesIn(u"one"_s).isEmpty());
    }

    void renameCannotLeaveTheVault()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;
        auto outside = QFileInfo(f.folder.path()).dir().filePath(escapee);
        f.write(u"one/a.txt"_s, "hello");

        auto renamed =
            f.vault.rename(f.absolute(u"one/a.txt"_s), u"../../"_s + escapee);

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

        auto old = f.absolute(u"one/a.txt"_s);

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
        QCOMPARE(model->fileRef().relative, Coco::Path("one/a.txt"));
    }

    void moveCannotLeaveTheVault()
    {
        Fixture_ f{};
        auto escapee = QFileInfo(f.folder.path()).fileName() + u"-out.txt"_s;
        auto outside = QFileInfo(f.folder.path()).dir().filePath(escapee);
        f.write(u"one/"_s + escapee, "hello");

        // The folder above the vault, written as a path under the vault
        auto moved = f.vault.move(
            f.absolute(u"one/"_s + escapee),
            f.absolute(u"one/../.."_s));

        auto left = QFileInfo::exists(outside);
        QFile::remove(outside);

        QVERIFY(moved.isEmpty());
        QVERIFY(!left);
        QVERIFY(f.exists(u"one/"_s + escapee));
    }

    void moveRefusesAPathThatIsNotPlain()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        f.makeFolder(u"one"_s);
        f.makeFolder(u"two"_s);

        // Both would land inside the vault
        QVERIFY(
            f.vault.move(f.absolute(u"a.txt"_s), f.absolute(u"two/../one"_s))
                .isEmpty());
        QVERIFY(
            f.vault.move(f.absolute(u"one/../a.txt"_s), f.absolute(u"one"_s))
                .isEmpty());

        QVERIFY(f.exists(u"a.txt"_s));
        QVERIFY(f.namesIn(u"one"_s).isEmpty());
    }

    // --- Moving -------------------------------------------------------------

    void moveAFileIntoAFolder()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        f.makeFolder(u"one"_s);
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        auto moved = f.vault.move(f.absolute(u"a.txt"_s), f.absolute(u"one"_s));

        QCOMPARE(moved, Coco::Path("one/a.txt"));
        QVERIFY(!f.exists(u"a.txt"_s));
        QCOMPARE(f.read(u"one/a.txt"_s), QByteArray("hello"));
        QCOMPARE(model->fileRef().relative, Coco::Path("one/a.txt"));
        QCOMPARE(f.open(u"one/a.txt"_s), model);
    }

    void moveAFileToTheRoot()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");

        auto moved = f.vault.move(f.absolute(u"one/a.txt"_s), f.root);

        QCOMPARE(moved, Coco::Path("a.txt"));
        QVERIFY(f.exists(u"a.txt"_s));
    }

    void moveAFolderWithOpenFilesInside()
    {
        Fixture_ f{};
        f.write(u"one/two/a.txt"_s, "hello");
        f.makeFolder(u"other"_s);
        auto* model = f.open(u"one/two/a.txt"_s);
        QVERIFY(model);

        auto moved = f.vault.move(f.absolute(u"one"_s), f.absolute(u"other"_s));

        QCOMPARE(moved, Coco::Path("other/one"));
        QCOMPARE(model->fileRef().relative, Coco::Path("other/one/two/a.txt"));
        QVERIFY(f.exists(u"other/one/two/a.txt"_s));
        QVERIFY(!f.exists(u"one"_s));
    }

    void moveAFolderIntoItselfIsRefused()
    {
        Fixture_ f{};
        f.write(u"one/two/a.txt"_s, "hello");

        QVERIFY(
            f.vault.move(f.absolute(u"one"_s), f.absolute(u"one"_s)).isEmpty());
        QVERIFY(f.vault.move(f.absolute(u"one"_s), f.absolute(u"one/two"_s))
                    .isEmpty());

        QVERIFY(f.exists(u"one/two/a.txt"_s));
    }

    void moveOntoATakenNameIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "top");
        f.write(u"one/a.txt"_s, "inner");

        QVERIFY(f.vault.move(f.absolute(u"a.txt"_s), f.absolute(u"one"_s))
                    .isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("top"));
        QCOMPARE(f.read(u"one/a.txt"_s), QByteArray("inner"));
    }

    void moveIntoSomethingThatIsNotAFolderIsRefused()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");

        QVERIFY(f.vault.move(f.absolute(u"a.txt"_s), f.absolute(u"b.txt"_s))
                    .isEmpty());
        QVERIFY(f.vault.move(f.absolute(u"a.txt"_s), f.absolute(u"gone"_s))
                    .isEmpty());

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
        QVERIFY(
            f.vault.move(f.absolute(u"a.txt"_s), Coco::Path(elsewhere.path()))
                .isEmpty());

        // Into the vault
        QVERIFY(f.vault.move(Coco::Path(elsewhere.filePath(u"b.txt"_s)), f.root)
                    .isEmpty());

        QVERIFY(f.exists(u"a.txt"_s));
        QVERIFY(!f.exists(u"b.txt"_s));
        QVERIFY(QFileInfo::exists(elsewhere.filePath(u"b.txt"_s)));
        QVERIFY(!QFileInfo::exists(elsewhere.filePath(u"a.txt"_s)));
    }

    // --- Flushing -----------------------------------------------------------

    void flushWritesAnEdit()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u" world"_s);
        QVERIFY(model->isModified());
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello"));

        QVERIFY(f.vault.flush().isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello world"));
        QVERIFY(!model->isModified());
    }

    void flushKeepsTheFilesLineEndingAndMark()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "\xEF\xBB\xBFone\r\n");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u"two\nthree"_s);
        QVERIFY(f.vault.flush().isEmpty());

        QCOMPARE(
            f.read(u"a.txt"_s),
            QByteArray("\xEF\xBB\xBFone\r\ntwo\r\nthree"));
    }

    // Shown by changing the file behind an unmodified buffer: a flush that
    // wrote would put the buffer's text back
    void flushWritesNothingForAnUnmodifiedBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        f.write(u"a.txt"_s, "changed outside");

        QVERIFY(f.vault.flush().isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("changed outside"));
    }

    void flushWritesOnlyTheModifiedBuffers()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        auto* a = f.open(u"a.txt"_s);
        auto* b = f.open(u"b.txt"_s);
        QVERIFY(a && b);

        type_(a, u"!"_s);
        f.write(u"b.txt"_s, "changed outside");

        QVERIFY(f.vault.flush().isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a!"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("changed outside"));
    }

    // A save only ever overwrites. A file deleted outside is not written
    // back, and that isn't reported as a failed save
    void flushDoesNotRecreateADeletedFile()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);
        type_(model, u"!"_s);

        QVERIFY(QFile::remove(f.folder.filePath(u"a.txt"_s)));

        QVERIFY(f.vault.flush().isEmpty());
        QVERIFY(!f.exists(u"a.txt"_s));
    }

    // Nor is a deleted folder rebuilt around it
    void flushDoesNotRecreateADeletedFolder()
    {
        Fixture_ f{};
        f.write(u"one/two/a.txt"_s, "hello");
        auto* model = f.open(u"one/two/a.txt"_s);
        QVERIFY(model);
        type_(model, u"!"_s);

        QVERIFY(QDir(f.folder.filePath(u"one"_s)).removeRecursively());

        QVERIFY(f.vault.flush().isEmpty());
        QVERIFY(!f.exists(u"one"_s));
    }

    // --- Freeing a buffer ---------------------------------------------------

    // A plain QObject stands in for a view: the buffer only counts its views
    // and watches for each one's destruction
    void closingTheLastViewSavesAndFreesTheBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy destroyed(model, &QObject::destroyed);

        {
            QObject view{};
            model->addView(&view);
            type_(model, u"!"_s);
        }

        // Saved at once, freed a moment later
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello!"));
        QTRY_COMPARE(destroyed.count(), 1);

        // A later open reads the file again, into a new buffer
        auto* reopened = f.open(u"a.txt"_s);
        QVERIFY(reopened);
        QCOMPARE(reopened->data(), QByteArray("hello!"));
        QVERIFY(!reopened->isUndoAvailable());
    }

    void closingOneOfTwoViewsKeepsTheBuffer()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QObject first{};
        model->addView(&first);

        {
            QObject second{};
            model->addView(&second);
            type_(model, u"!"_s);
        }

        // Nothing is saved or freed while a view remains
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello"));
        QVERIFY(model->isModified());
        QCOMPARE(f.open(u"a.txt"_s), model);
    }

    // The file was deleted outside, so there is nowhere to save: the last
    // view closing writes nothing
    void closingTheLastViewDoesNotRecreateADeletedFile()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "hello");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        {
            QObject view{};
            model->addView(&view);
            type_(model, u"!"_s);

            QVERIFY(QFile::remove(f.folder.filePath(u"a.txt"_s)));
        }

        QVERIFY(!f.exists(u"a.txt"_s));
    }

    // --- Settings -----------------------------------------------------------

    void aSettingIsAnnouncedOnceAndSavedByFlush()
    {
        QTemporaryDir folder{};
        Coco::Path root(folder.path());

        {
            Vault vault(root, nullptr);
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

            vault.flush();
        }

        // A second vault on the same folder reads what the first saved
        Vault reopened(root, nullptr);

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

        {
            Vault first(root, nullptr);
        }

        QFile file(folder.filePath(u".suzuri/dictionary.txt"_s));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Pangloss\nCunegonde\n");
        file.close();

        Vault vault(root, nullptr);

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
            f.read(u".suzuri/dictionary.txt"_s),
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

        f.write(u".suzuri/dictionary.txt"_s, "Candide\n");
        f.vault.addToDictionary(u"Cunegonde"_s);

        QCOMPARE(
            f.read(u".suzuri/dictionary.txt"_s),
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
        QVERIFY(!f.exists(u".suzuri/dictionary.txt"_s));
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

        QVERIFY(
            !f.vault.moveToTrash(Coco::Path(elsewhere.filePath(u"b.txt"_s))));
        QVERIFY(!f.vault.moveToTrash(f.root));
        QVERIFY(!f.vault.moveToTrash(f.absolute(u"gone.txt"_s)));

        // The root and a file, by paths that are not plain
        QVERIFY(!f.vault.moveToTrash(Coco::Path(f.folder.path() + u"/"_s)));
        QVERIFY(!f.vault.moveToTrash(Coco::Path(f.folder.path() + u"/."_s)));
        QVERIFY(!f.vault.moveToTrash(f.absolute(u"one/.."_s)));
        QVERIFY(!f.vault.moveToTrash(f.absolute(u"one/../a.txt"_s)));

        QVERIFY(QFileInfo::exists(elsewhere.filePath(u"b.txt"_s)));
        QVERIFY(QFileInfo(f.folder.path()).isDir());
        QVERIFY(f.exists(u"a.txt"_s));
    }
};

QTEST_MAIN(VaultTest)

#include "VaultTest.moc"
