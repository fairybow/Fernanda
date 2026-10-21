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

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/PathKey.h"
#include "models/AbstractFileModel.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using Fernanda::AbstractFileModel;
using Fernanda::BufferStore;
using Fernanda::PathKey;
using Fernanda::TextFileModel;

// What the buffer store does with open files (core/BufferStore.h), for
// everything that happens at once: opening, flushing, freeing, and following
// a vault's file operations. What waits on a timer or the file watcher
// (autosave, external changes) is BufferStoreTimedTest's. Renames, moves, and
// trashes are driven through a Vault in VaultTest, which checks that buffers
// follow.
//
// Each test gets its own store, and its files in a temporary folder.
//
// Qt Test runs every private slot as a test
class BufferStoreTest : public QObject
{
    Q_OBJECT

private:
    // A store, a temporary folder, and the file work a test does around them.
    // Paths given to these functions are relative to the folder.
    //
    // The store is declared after the folder so it is destroyed first
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };
        BufferStore store{ nullptr };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return root / Coco::Path(relative);
        }

        [[nodiscard]] PathKey key(const QString& relative) const
        {
            return PathKey::of(absolute(relative));
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
    // --- Opening ------------------------------------------------------------

    void openGivesABufferHoldingTheFile()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello\n");

        auto* model = f.open(u"one/a.txt"_s);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray("hello\n"));
        QCOMPARE(model->key(), f.key(u"one/a.txt"_s));
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

    void anotherSpellingOpensTheSameBuffer()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "hello");

        auto* model = f.open(u"one/a.txt"_s);

        QVERIFY(model);
        QCOMPARE(f.open(u"one/../one/a.txt"_s), model);
        QCOMPARE(f.open(u"./one/a.txt"_s), model);
        QCOMPARE(f.open(u"one//a.txt"_s), model);
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
        QVERIFY(qobject_cast<Fernanda::ImageFileModel*>(image));
        QVERIFY(!image->isUserEditable());
        QCOMPARE(image->data(), pngBytes_());

        auto* pdf = f.open(u"a.pdf"_s);
        QVERIFY(qobject_cast<Fernanda::PdfFileModel*>(pdf));
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

    // Which files a window may open is the window's to decide; the store
    // refuses only what isn't a readable file
    void openRefusesWhatIsNotAFile()
    {
        Fixture_ f{};
        f.makeFolder(u"one"_s);

        QVERIFY(!f.open(PathKey{}));
        QVERIFY(!f.open(u"one"_s));
        QVERIFY(!f.open(u"gone.txt"_s));
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
        PathKey asked_about{};
        QByteArray asked_with{};

        auto* model = f.store.openModel(
            f.key(u"a.txt"_s),
            [&](const PathKey& key, const QByteArray& data) {
                ++asked;
                asked_about = key;
                asked_with = data;
                return false;
            });

        QVERIFY(!model);
        QCOMPARE(asked, 1);
        QCOMPARE(asked_about, f.key(u"a.txt"_s));
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
        auto accept = [&asked](const PathKey&, const QByteArray&) {
            ++asked;
            return true;
        };

        auto* model = f.store.openModel(f.key(u"a.txt"_s), accept);

        QVERIFY(model);
        QCOMPARE(model->data(), QByteArray("caf\xEF\xBF\xBD"));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("caf\xEF\xBF\xBD"));
        QVERIFY(!model->isModified());

        // The open buffer answers a second open, and nothing is asked
        QCOMPARE(f.store.openModel(f.key(u"a.txt"_s), accept), model);
        QCOMPARE(asked, 1);
    }

    // Only text is asked about: other types are never decoded
    void openNeverAsksAboutAnImage()
    {
        Fixture_ f{};
        f.write(u"a.png"_s, "\xFF\xFE not an image");

        QVERIFY(f.open(u"a.png"_s));
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

        QVERIFY(f.store.flush().isEmpty());

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
        QVERIFY(f.store.flush().isEmpty());

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

        QVERIFY(f.store.flush().isEmpty());

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

        QVERIFY(f.store.flush().isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a!"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("changed outside"));
    }

    // A closing window flushes only the files it shows
    void flushOfSomeFilesWritesOnlyThose()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        auto* a = f.open(u"a.txt"_s);
        auto* b = f.open(u"b.txt"_s);
        QVERIFY(a && b);

        type_(a, u"!"_s);
        type_(b, u"!"_s);

        QVERIFY(f.store.flush(QList<PathKey>{ f.key(u"a.txt"_s) }).isEmpty());

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a!"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("b"));
        QVERIFY(b->isModified());
    }

    // A file no buffer holds, or one named twice, is passed over
    void flushOfSomeFilesPassesOverWhatIsNotOpen()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        auto* a = f.open(u"a.txt"_s);
        QVERIFY(a);
        type_(a, u"!"_s);

        auto failed = f.store.flush(
            QList<PathKey>{ f.key(u"a.txt"_s),
                            f.key(u"a.txt"_s),
                            f.key(u"never-opened.txt"_s) });

        QVERIFY(failed.isEmpty());
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a!"));
        QVERIFY(!f.exists(u"never-opened.txt"_s));
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

        QVERIFY(f.store.flush().isEmpty());
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

        QVERIFY(f.store.flush().isEmpty());
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

    // --- Following a vault's file operations --------------------------------

    void applyRelocationRekeysEveryBufferAtOrUnder()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        f.write(u"one/two/b.txt"_s, "b");
        f.write(u"other.txt"_s, "c");
        auto* a = f.open(u"one/a.txt"_s);
        auto* b = f.open(u"one/two/b.txt"_s);
        auto* other = f.open(u"other.txt"_s);
        QVERIFY(a && b && other);

        QSignalSpy renamed(a, &AbstractFileModel::renamed);

        f.store.applyRelocation(f.key(u"one"_s), f.key(u"uno"_s));

        QCOMPARE(renamed.count(), 1);
        QCOMPARE(a->key(), f.key(u"uno/a.txt"_s));
        QCOMPARE(b->key(), f.key(u"uno/two/b.txt"_s));
        QCOMPARE(other->key(), f.key(u"other.txt"_s));
    }

    // The relocation is announced once, after every buffer under the entry
    // took its new key, so whoever hears it sees the store as it now is
    void applyRelocationIsAnnouncedAfterTheBuffersFollow()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        auto* a = f.open(u"one/a.txt"_s);
        QVERIFY(a);

        QList<PathKey> heard{};

        QObject::connect(
            &f.store,
            &BufferStore::entryRelocated,
            &f.store,
            [&heard, a](const PathKey& oldEntry, const PathKey& newEntry) {
                heard << oldEntry << newEntry << a->key();
            });

        f.store.applyRelocation(f.key(u"one"_s), f.key(u"uno"_s));

        QCOMPARE(heard.size(), 3);
        QVERIFY(heard[0] == f.key(u"one"_s));
        QVERIFY(heard[1] == f.key(u"uno"_s));
        QVERIFY(heard[2] == f.key(u"uno/a.txt"_s));
    }

    // Only the modified buffers under the entry are written
    void flushAtOrUnderWritesOnlyThatEntry()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        f.write(u"other.txt"_s, "c");
        auto* a = f.open(u"one/a.txt"_s);
        auto* other = f.open(u"other.txt"_s);
        QVERIFY(a && other);
        type_(a, u"!"_s);
        type_(other, u"!"_s);

        QVERIFY(f.store.flushAtOrUnder(f.key(u"one"_s)));

        QCOMPARE(f.read(u"one/a.txt"_s), QByteArray("a!"));
        QCOMPARE(f.read(u"other.txt"_s), QByteArray("c"));
    }

    // Every view on a discarded buffer is told, and the buffer is freed
    void applyRemovalFreesThoseBuffers()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        f.write(u"other.txt"_s, "c");
        auto* a = f.open(u"one/a.txt"_s);
        auto* other = f.open(u"other.txt"_s);
        QVERIFY(a && other);

        QSignalSpy removed(a, &AbstractFileModel::removedFromDisk);
        QSignalSpy destroyed(a, &QObject::destroyed);

        QCOMPARE(f.store.applyRemoval(f.key(u"one"_s)), 1);

        QCOMPARE(removed.count(), 1);
        QTRY_COMPARE(destroyed.count(), 1);
        QCOMPARE(f.open(u"other.txt"_s), other);
    }

    // The removal is announced once, after every buffer under the entry was
    // let go
    void applyRemovalIsAnnouncedAfterTheBuffersGo()
    {
        Fixture_ f{};
        f.write(u"one/a.txt"_s, "a");
        auto* a = f.open(u"one/a.txt"_s);
        QVERIFY(a);

        auto a_removed = false;
        auto a_removed_when_heard = false;
        QList<PathKey> heard{};

        QObject::connect(
            a,
            &AbstractFileModel::removedFromDisk,
            &f.store,
            [&a_removed] { a_removed = true; });

        QObject::connect(
            &f.store,
            &BufferStore::entryRemoved,
            &f.store,
            [&heard, &a_removed, &a_removed_when_heard](const PathKey& entry) {
                a_removed_when_heard = a_removed;
                heard << entry;
            });

        f.store.applyRemoval(f.key(u"one"_s));

        QCOMPARE(heard.size(), 1);
        QVERIFY(heard[0] == f.key(u"one"_s));
        QVERIFY(a_removed_when_heard);
    }
};

QTEST_MAIN(BufferStoreTest)

#include "BufferStoreTest.moc"
