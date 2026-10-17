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
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QSaveFile>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <Coco/Path.h>

#include "core/Vault.h"
#include "models/AbstractFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using Suzuri::AbstractFileModel;
using Suzuri::TextFileModel;
using Suzuri::Vault;

// What a Vault does a moment later (core/Vault.h): saving after typing, and
// taking in changes made to its open files by other programs. Both run on
// real timers and the real file watcher, so these tests take real time.
//
// Something expected to happen is waited for with QTRY_COMPARE and
// QTRY_VERIFY, which return as soon as it does and fail after five seconds.
// Something expected NOT to happen is given SETTLE_MS_, and then checked. A
// slow machine can only make the second kind pass when it shouldn't, never
// fail when it shouldn't.
//
// Each test gets its own vault in a temporary folder.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class VaultTimedTest : public QObject
{
    Q_OBJECT

private:
    // Longer than the vault's wait before it handles file changes, with room
    // for the watcher to report them
    static constexpr int SETTLE_MS_ = 700;

    // Far enough off that the timer it is given to can't fire during a test
    static constexpr int NEVER_MS_ = 60 * 60 * 1000;

    // A vault in a temporary folder, and the file work a test does around it.
    // Paths given to these functions are relative to the vault's root.
    //
    // The vault is declared after the folder so it is destroyed first
    struct Fixture_
    {
        QTemporaryDir folder{};
        Coco::Path root{ folder.path() };
        Vault vault{ root, nullptr };

        // Written the way most editors and sync tools save: to a temporary
        // file that then replaces the original
        void replace(const QString& relative, const QByteArray& bytes) const
        {
            QFileInfo info(folder.filePath(relative));
            QDir().mkpath(info.path());

            QSaveFile file(info.filePath());
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(bytes);
            QVERIFY(file.commit());
        }

        // Written into the existing file, which stays the same file
        void overwrite(const QString& relative, const QByteArray& bytes) const
        {
            QFile file(folder.filePath(relative));
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
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

        [[nodiscard]] AbstractFileModel* open(const QString& relative)
        {
            return vault.openModel(
                Coco::Path(relative),
                [](const Coco::Path&, const QByteArray&) { return false; });
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

    [[nodiscard]] static QByteArray pngBytes_(Qt::GlobalColor color)
    {
        QImage image(2, 2, QImage::Format_RGB32);
        image.fill(color);

        QByteArray bytes{};
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");

        return bytes;
    }

private slots:
    // --- Saving after typing ------------------------------------------------

    // The debounce: a save follows a pause in typing
    void typingIsSavedAfterAPause()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "hello");
        f.vault.setAutosaveTiming(100, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u"!"_s);

        // Not at once
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello"));

        QTRY_COMPARE(f.read(u"a.txt"_s), QByteArray("hello!"));
        QVERIFY(!model->isModified());
    }

    // The ceiling: a save comes anyway when the pause never does. The
    // debounce is set out of reach, so only the ceiling can have saved
    void typingIsSavedWithoutAPause()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "hello");
        f.vault.setAutosaveTiming(NEVER_MS_, 300);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u"!"_s);

        QTRY_COMPARE(f.read(u"a.txt"_s), QByteArray("hello!"));
        QVERIFY(!model->isModified());
    }

    // The ceiling starts again with the next burst of typing
    void theNextBurstOfTypingIsSavedToo()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "hello");
        f.vault.setAutosaveTiming(NEVER_MS_, 200);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u"!"_s);
        QTRY_COMPARE(f.read(u"a.txt"_s), QByteArray("hello!"));

        type_(model, u"?"_s);
        QTRY_COMPARE(f.read(u"a.txt"_s), QByteArray("hello!?"));
    }

    // One pair of timers serves the whole vault: one save pass writes every
    // modified buffer
    void oneSaveWritesEveryModifiedBuffer()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "a");
        f.replace(u"one/b.txt"_s, "b");
        f.vault.setAutosaveTiming(100, NEVER_MS_);

        auto* a = f.open(u"a.txt"_s);
        auto* b = f.open(u"one/b.txt"_s);
        QVERIFY(a && b);

        type_(a, u"!"_s);
        type_(b, u"?"_s);

        QTRY_COMPARE(f.read(u"a.txt"_s), QByteArray("a!"));
        QTRY_COMPARE(f.read(u"one/b.txt"_s), QByteArray("b?"));
    }

    // --- A change made by another program -----------------------------------

    void anOutsideChangeIsTakenIn_data()
    {
        QTest::addColumn<bool>("byReplacing");

        QTest::newRow("file replaced") << true;
        QTest::newRow("file overwritten in place") << false;
    }

    void anOutsideChangeIsTakenIn()
    {
        QFETCH(bool, byReplacing);

        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy reloaded(model, &AbstractFileModel::reloaded);

        if (byReplacing) {
            f.replace(u"a.txt"_s, "two\n");
        } else {
            f.overwrite(u"a.txt"_s, "two\n");
        }

        QTRY_COMPARE(model->data(), QByteArray("two\n"));
        QTRY_COMPARE(reloaded.count(), 1);

        // The buffer matches disk, so there is nothing to save
        QVERIFY(!model->isModified());

        // And the change can be undone
        model->undo();
        QCOMPARE(model->data(), QByteArray("one\n"));
    }

    // Replacing a file drops it from the watcher. If the vault didn't watch
    // it again, the first outside change would be the last one seen
    void aSecondOutsideChangeIsTakenInToo()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        f.replace(u"a.txt"_s, "two\n");
        QTRY_COMPARE(model->data(), QByteArray("two\n"));

        f.replace(u"a.txt"_s, "three\n");
        QTRY_COMPARE(model->data(), QByteArray("three\n"));

        f.replace(u"a.txt"_s, "four\n");
        QTRY_COMPARE(model->data(), QByteArray("four\n"));
    }

    // The outside change wins, and the typing it replaced is one undo away
    void anOutsideChangeOverUnsavedTypingCanBeUndone()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(NEVER_MS_, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);
        type_(model, u"unsaved"_s);

        f.replace(u"a.txt"_s, "two\n");
        QTRY_COMPARE(model->data(), QByteArray("two\n"));

        model->undo();
        QCOMPARE(model->data(), QByteArray("one\nunsaved"));
    }

    // A change that leaves the file holding what the buffer already holds is
    // not a reload
    void anOutsideChangeToTheSameBytesIsIgnored()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy reloaded(model, &AbstractFileModel::reloaded);

        f.replace(u"a.txt"_s, "one\n");
        QTest::qWait(SETTLE_MS_);

        QCOMPARE(reloaded.count(), 0);
        QVERIFY(!model->isUndoAvailable());
    }

    void anOutsideChangeToAnImageIsTakenIn()
    {
        Fixture_ f{};
        f.replace(u"a.png"_s, pngBytes_(Qt::red));
        auto* model = f.open(u"a.png"_s);
        QVERIFY(model);

        QSignalSpy reloaded(model, &AbstractFileModel::reloaded);

        f.replace(u"a.png"_s, pngBytes_(Qt::blue));

        QTRY_COMPARE(model->data(), pngBytes_(Qt::blue));
        QTRY_COMPARE(reloaded.count(), 1);
    }

    // --- The vault's own saves ----------------------------------------------

    // A save is a change to a watched file, and must not come back as an
    // outside change. The danger is typing done after the save: a reload
    // would put the saved text back over it
    void ownSaveIsNotTakenForAnOutsideChange()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(NEVER_MS_, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy reloaded(model, &AbstractFileModel::reloaded);

        type_(model, u"two"_s);
        QVERIFY(f.vault.flush().isEmpty());

        // The buffer moves on before the watcher has reported the save
        type_(model, u" three"_s);

        QTest::qWait(SETTLE_MS_);

        QCOMPARE(reloaded.count(), 0);
        QCOMPARE(model->data(), QByteArray("one\ntwo three"));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("one\ntwo"));
        QVERIFY(model->isModified());
    }

    // The same, with the report forced: whether a save of the vault's own is
    // reported at all depends on the platform, so here another program
    // writes the file again with the very bytes the vault last saved. The
    // vault knows those bytes as its own and leaves the buffer alone. Judged
    // against the buffer instead, they would look like an outside change,
    // and the later typing would be lost
    void bytesTheVaultLastSavedAreNeverTakenIn()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(NEVER_MS_, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy reloaded(model, &AbstractFileModel::reloaded);

        type_(model, u"two"_s);
        QVERIFY(f.vault.flush().isEmpty());
        type_(model, u" three"_s);

        f.replace(u"a.txt"_s, "one\ntwo");
        QTest::qWait(SETTLE_MS_);

        QCOMPARE(reloaded.count(), 0);
        QCOMPARE(model->data(), QByteArray("one\ntwo three"));
        QVERIFY(model->isModified());
    }

    // After its own save the vault still sees a real outside change
    void anOutsideChangeAfterOwnSaveIsTakenIn()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(NEVER_MS_, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        type_(model, u"two"_s);
        QVERIFY(f.vault.flush().isEmpty());
        QTest::qWait(SETTLE_MS_);

        f.replace(u"a.txt"_s, "outside\n");

        QTRY_COMPARE(model->data(), QByteArray("outside\n"));
    }

    // --- A file deleted by another program ----------------------------------

    void anOutsideDeleteClosesTheBuffer()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy removed(model, &AbstractFileModel::removedFromDisk);
        QSignalSpy destroyed(model, &QObject::destroyed);

        QVERIFY(QFile::remove(f.folder.filePath(u"a.txt"_s)));

        QTRY_COMPARE(removed.count(), 1);
        QTRY_COMPARE(destroyed.count(), 1);

        QVERIFY(!f.open(u"a.txt"_s));
        QVERIFY(!f.exists(u"a.txt"_s));
    }

    // Unsaved typing doesn't bring the file back
    void anOutsideDeleteIsNotUndoneByUnsavedTyping()
    {
        Fixture_ f{};
        f.replace(u"a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(100, NEVER_MS_);

        auto* model = f.open(u"a.txt"_s);
        QVERIFY(model);

        QSignalSpy removed(model, &AbstractFileModel::removedFromDisk);

        type_(model, u"unsaved"_s);
        QVERIFY(QFile::remove(f.folder.filePath(u"a.txt"_s)));

        QTRY_COMPARE(removed.count(), 1);

        // Past when the autosave would have run
        QTest::qWait(SETTLE_MS_);
        QVERIFY(!f.exists(u"a.txt"_s));
    }

    // Deleting a folder may send no change for the files inside it. The next
    // save notices the file is gone, and closes the buffer the same way
    void anOutsideFolderDeleteClosesTheBuffersInside()
    {
        Fixture_ f{};
        f.replace(u"one/two/a.txt"_s, "one\n");
        f.vault.setAutosaveTiming(NEVER_MS_, NEVER_MS_);

        auto* model = f.open(u"one/two/a.txt"_s);
        QVERIFY(model);

        QSignalSpy removed(model, &AbstractFileModel::removedFromDisk);

        type_(model, u"unsaved"_s);
        QVERIFY(QDir(f.folder.filePath(u"one"_s)).removeRecursively());

        QVERIFY(f.vault.flush().isEmpty());

        QTRY_COMPARE(removed.count(), 1);
        QVERIFY(!f.exists(u"one"_s));
    }
};

QTEST_MAIN(VaultTimedTest)

#include "VaultTimedTest.moc"
