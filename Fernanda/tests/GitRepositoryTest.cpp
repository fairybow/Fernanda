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

#include <algorithm>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QProcess>
#include <QStandardPaths>
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
#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "models/AbstractFileModel.h"
#include "models/TextFileModel.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Finding a vault's repository and reading its status by running git
// (core/git/GitRepository.h). These run the git installed on the machine,
// and are skipped where there is none. What the status output means is
// GitStatusTest's; these check that git is asked the right way, and that
// what the store holds unsaved is saved before it is asked.
//
// Each test makes its own repository in a temporary folder.
//
// Qt Test runs every private slot as a test
class GitRepositoryTest : public QObject
{
    Q_OBJECT

private:
    // How long a test waits for something that should not happen
    static constexpr int QUIET_WAIT_MS_ = 1000;

    // A temporary folder, which a test may make a repository, and the store
    // and vault a GitRepository borrows. vaultFolder is where the vault is,
    // relative to the folder. Paths given to these functions are relative to
    // the folder.
    //
    // Declared in the order they're made, so each is destroyed before what it
    // depends on
    struct Fixture_
    {
        explicit Fixture_(const QString& vaultFolder = {})
            : vaultRoot(makeVaultRoot_(folder, vaultFolder))
        {
        }

        QTemporaryDir folder{};
        Coco::Path vaultRoot;
        BufferStore store{ nullptr };
        Vault vault{ vaultRoot, &store, nullptr };
        GitRepository repository{ &vault, &store, nullptr };

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return Coco::Path(folder.path()) / Coco::Path(relative);
        }

        // Runs git in the folder, and fails the test if git does. Commits are
        // made as a fixed person, and unsigned whatever the machine's own
        // configuration says
        void git(const QStringList& arguments) const
        {
            QProcess process{};
            process.setWorkingDirectory(folder.path());
            process.start(
                u"git"_s,
                QStringList{
                    u"-c"_s,
                    u"user.name=Test"_s,
                    u"-c"_s,
                    u"user.email=test@example.com"_s,
                    u"-c"_s,
                    u"commit.gpgsign=false"_s,
                } + arguments);

            QVERIFY(process.waitForFinished());

            if (process.exitCode() != 0) {
                QFAIL(qPrintable(u"git %1 failed: %2"_s.arg(
                    arguments.join(u' '),
                    QString::fromUtf8(process.readAllStandardError()))));
            }
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

        // The folder as git reports it: with any symlink above it resolved,
        // as on macOS, where the temporary folder is under one
        [[nodiscard]] PathKey canonicalKey() const
        {
            return PathKey::of(Coco::Path(folder.path()).canonical());
        }

    private:
        [[nodiscard]] static Coco::Path
        makeVaultRoot_(const QTemporaryDir& folder, const QString& vaultFolder)
        {
            QDir(folder.path())
                .mkpath(vaultFolder.isEmpty() ? u"."_s : vaultFolder);

            return Coco::Path(folder.path()) / Coco::Path(vaultFolder);
        }
    };

    // Whether git finds a repository from the folder, asked directly
    [[nodiscard]] static bool isInARepository_(const QString& folder)
    {
        QProcess process{};
        process.setWorkingDirectory(folder);
        process.start(u"git"_s, { u"rev-parse"_s, u"--show-toplevel"_s });
        process.waitForFinished();

        return process.exitStatus() == QProcess::NormalExit &&
               process.exitCode() == 0;
    }

    // The status entry for a path relative to the repository, or nullptr
    [[nodiscard]] static const GitStatus::Entry*
    entry_(const GitRepository& repository, const QString& path)
    {
        const auto& entries = repository.status().entries;

        auto it = std::find_if(
            entries.cbegin(),
            entries.cend(),
            [&path](const GitStatus::Entry& entry) {
                return entry.path.prettyQString() == path;
            });

        return it == entries.cend() ? nullptr : &*it;
    }

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
    void initTestCase()
    {
        if (QStandardPaths::findExecutable(u"git"_s).isEmpty()) {
            QSKIP("Git isn't installed");
        }
    }

    // --- Finding the repository ---------------------------------------------

    void findsARepositoryAtTheVaultsFolder()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });

        QVERIFY(f.repository.hasGit());
        QVERIFY(!f.repository.isFound());

        f.repository.detect();

        QTRY_VERIFY(f.repository.isFound());
        QCOMPARE(PathKey::of(f.repository.root()), f.canonicalKey());
    }

    void findsARepositoryAboveTheVaultsFolder()
    {
        Fixture_ f{ u"Novels/Candide"_s };
        f.git({ u"init"_s, u"-q"_s });

        f.repository.detect();

        QTRY_VERIFY(f.repository.isFound());
        QCOMPARE(PathKey::of(f.repository.root()), f.canonicalKey());
    }

    // The temporary folder could itself be inside a repository (a home
    // folder kept in one), in which case there is nothing to test here or in
    // losingTheRepositoryIsAnnounced
    void findsNoRepositoryOutsideOne()
    {
        Fixture_ f{};

        if (isInARepository_(f.folder.path())) {
            QSKIP("The temporary folder is inside a repository");
        }

        f.repository.detect();
        QTest::qWait(QUIET_WAIT_MS_);

        QVERIFY(!f.repository.isFound());
        QVERIFY(f.repository.root().isEmpty());
    }

    void losingTheRepositoryIsAnnounced()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        QVERIFY(QDir(f.folder.filePath(u".git"_s)).removeRecursively());

        if (isInARepository_(f.folder.path())) {
            QSKIP("The temporary folder is inside a repository");
        }

        f.repository.detect();

        QTRY_VERIFY(!f.repository.isFound());
        QVERIFY(f.repository.status().entries.isEmpty());
    }

    // --- Status -------------------------------------------------------------

    void refreshListsAnUntrackedFile()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"Chapter One/a b.txt"_s, "hello");

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        f.repository.refresh();

        QTRY_VERIFY(entry_(f.repository, u"Chapter One/a b.txt"_s));
        QCOMPARE(
            entry_(f.repository, u"Chapter One/a b.txt"_s)->change,
            GitStatus::Change::Untracked);
    }

    // "Café" is a name with an e-acute: git must hand it back as the
    // same name, not quoted or escaped
    void refreshReadsANameOutsideAscii()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"Café.txt"_s, "hello");

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        f.repository.refresh();

        QTRY_VERIFY(entry_(f.repository, u"Café.txt"_s));
    }

    void refreshReadsTheBranch()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "hello");
        f.git({ u"add"_s, u"a.txt"_s });
        f.git({ u"commit"_s, u"-q"_s, u"-m"_s, u"First"_s });
        f.git({ u"checkout"_s, u"-q"_s, u"-b"_s, u"draft-two"_s });

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        f.repository.refresh();

        QTRY_COMPARE(f.repository.status().branch.name, u"draft-two"_s);
        QVERIFY(!f.repository.status().branch.unborn);
        QVERIFY(!f.repository.status().branch.detached);
    }

    // The status reads the disk, so a refresh saves what is typed first
    void refreshSavesUnsavedTypingFirst()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "hello");
        f.git({ u"add"_s, u"a.txt"_s });
        f.git({ u"commit"_s, u"-q"_s, u"-m"_s, u"First"_s });

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        auto* model = f.store.openModel(
            PathKey::of(f.absolute(u"a.txt"_s)),
            [](const PathKey&, const QByteArray&) { return false; });
        QVERIFY(model);

        type_(model, u" world"_s);
        QVERIFY(model->isModified());

        f.repository.refresh();

        QCOMPARE(f.read(u"a.txt"_s), QByteArray("hello world"));
        QTRY_VERIFY(entry_(f.repository, u"a.txt"_s));
        QCOMPARE(
            entry_(f.repository, u"a.txt"_s)->change,
            GitStatus::Change::Modified);
    }

    // Asking again while a status waits adds nothing, and the waiting one
    // still reads the file written after it was asked for
    void refreshesAskedTogetherReadTheLatestState()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());

        f.repository.refresh();
        f.repository.refresh();
        f.write(u"late.txt"_s, "late");
        f.repository.refresh();

        QTRY_VERIFY(entry_(f.repository, u"late.txt"_s));
    }
};

QTEST_MAIN(GitRepositoryTest)
#include "GitRepositoryTest.moc"
