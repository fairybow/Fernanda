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
#include <optional>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QProcess>
#include <QSignalSpy>
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
#include "core/git/GitDiff.h"
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

        // The folder as the vault spells it, which the repository's root
        // must match even where git would resolve a symlink above it
        [[nodiscard]] PathKey folderKey() const
        {
            return PathKey::of(Coco::Path(folder.path()));
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

    // Finds the repository, reads its status, and asks for the diff of the
    // changed file at path. Nothing if the file isn't listed as changed, or
    // git fails
    [[nodiscard]] static std::optional<GitDiff::Diff>
    diffOf_(Fixture_& f, const QString& path)
    {
        f.repository.detect();

        if (!QTest::qWaitFor([&] { return f.repository.isFound(); })) {
            return std::nullopt;
        }

        f.repository.refresh();

        if (!QTest::qWaitFor([&] { return entry_(f.repository, path); })) {
            return std::nullopt;
        }

        std::optional<GitDiff::Diff> answer{};
        auto failed = false;

        auto read = connect(
            &f.repository,
            &GitRepository::diffRead,
            [&answer](const Coco::Path&, const GitDiff::Diff& diff) {
                answer = diff;
            });

        auto fail = connect(
            &f.repository,
            &GitRepository::diffFailed,
            [&failed](const Coco::Path&, const QString&) { failed = true; });

        f.repository.readDiff(*entry_(f.repository, path));
        [[maybe_unused]] auto done =
            QTest::qWaitFor([&] { return answer || failed; });

        disconnect(read);
        disconnect(fail);
        return answer;
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
        QCOMPARE(PathKey::of(f.repository.root()), f.folderKey());
    }

    void findsARepositoryAboveTheVaultsFolder()
    {
        Fixture_ f{ u"Novels/Candide"_s };
        f.git({ u"init"_s, u"-q"_s });

        f.repository.detect();

        QTRY_VERIFY(f.repository.isFound());
        QCOMPARE(PathKey::of(f.repository.root()), f.folderKey());
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

    void forgetDropsTheRepository()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "hello");

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());
        f.repository.refresh();
        QTRY_VERIFY(!f.repository.status().entries.isEmpty());

        QSignalSpy found_changed(&f.repository, &GitRepository::foundChanged);
        f.repository.forget();

        QCOMPARE(found_changed.count(), 1);
        QVERIFY(!f.repository.isFound());
        QVERIFY(f.repository.status().entries.isEmpty());
    }

    // The detection is running when forget comes, so its answer arrives
    // after, and must not bring the repository back
    void forgetIgnoresAnAnswerStillToCome()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });

        f.repository.detect();
        f.repository.forget();
        QTest::qWait(QUIET_WAIT_MS_);

        QVERIFY(!f.repository.isFound());

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());
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

    // Git reports a folder with the symlinks above it resolved. The root must
    // keep the vault's spelling, or a refresh can't find the buffers to save:
    // their keys are made under the vault's folder as it was given. macOS's
    // temporary folders sit under a symlink; this makes one everywhere it can
    // be made without special permission
    void refreshSavesTypingUnderASymlinkedFolder()
    {
#ifdef Q_OS_WIN
        QSKIP("Making a symlink needs special permission on Windows");
#else
        QTemporaryDir folder{};
        QDir(folder.path()).mkpath(u"Real/Vault"_s);
        QVERIFY(
            QFile::link(
                folder.filePath(u"Real"_s),
                folder.filePath(u"Link"_s)));

        auto real = Coco::Path(folder.path()) / Coco::Path(u"Real"_s);
        auto vault_root =
            Coco::Path(folder.path()) / Coco::Path(u"Link/Vault"_s);

        QProcess init{};
        init.setWorkingDirectory(real.toQString());
        init.start(u"git"_s, { u"init"_s, u"-q"_s });
        QVERIFY(init.waitForFinished());
        QCOMPARE(init.exitCode(), 0);

        QFile file((vault_root / Coco::Path(u"a.txt"_s)).toQString());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("hello");
        file.close();

        BufferStore store{ nullptr };
        Vault vault{ vault_root, &store, nullptr };
        GitRepository repository{ &vault, &store, nullptr };

        repository.detect();
        QTRY_VERIFY(repository.isFound());
        QCOMPARE(
            PathKey::of(repository.root()),
            PathKey::of(Coco::Path(folder.path()) / Coco::Path(u"Link"_s)));

        auto* model = store.openModel(
            PathKey::of(vault_root / Coco::Path(u"a.txt"_s)),
            [](const PathKey&, const QByteArray&) { return false; });
        QVERIFY(model);

        type_(model, u" world"_s);
        repository.refresh();

        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("hello world"));
#endif
    }

    // --- Diffs --------------------------------------------------------------

    void readDiffShowsAChangedLine()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "one\ntwo\n");
        f.git({ u"add"_s, u"a.txt"_s });
        f.git({ u"commit"_s, u"-q"_s, u"-m"_s, u"First"_s });
        f.write(u"a.txt"_s, "one\nTwo\n");

        auto diff = diffOf_(f, u"a.txt"_s);
        QVERIFY(diff);
        QVERIFY(!diff->binary);
        QCOMPARE(diff->hunks.size(), 1);

        const auto& lines = diff->hunks[0].lines;
        QCOMPARE(lines.size(), 3);
        QCOMPARE(lines[1].kind, GitDiff::Line::Kind::Removed);
        QCOMPARE(lines[1].text, u"two"_s);
        QCOMPARE(lines[2].kind, GitDiff::Line::Kind::Added);
        QCOMPARE(lines[2].text, u"Two"_s);
    }

    void readDiffShowsAnUntrackedFileAsAdded()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "first\n");
        f.git({ u"add"_s, u"a.txt"_s });
        f.git({ u"commit"_s, u"-q"_s, u"-m"_s, u"First"_s });
        f.write(u"Chapter One/new.md"_s, "one\ntwo\n");

        auto diff = diffOf_(f, u"Chapter One/new.md"_s);
        QVERIFY(diff);
        QCOMPARE(diff->hunks.size(), 1);
        QCOMPARE(diff->hunks[0].lines.size(), 2);
        QCOMPARE(diff->hunks[0].lines[0].kind, GitDiff::Line::Kind::Added);
        QCOMPARE(diff->hunks[0].lines[1].text, u"two"_s);
    }

    // There is no HEAD to compare with, so every file is compared with
    // nothing
    void readDiffWorksBeforeTheFirstCommit()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "hello\n");
        f.git({ u"add"_s, u"a.txt"_s });

        auto diff = diffOf_(f, u"a.txt"_s);
        QVERIFY(diff);
        QCOMPARE(diff->hunks.size(), 1);
        QCOMPARE(diff->hunks[0].lines[0].text, u"hello"_s);
    }

    void readDiffComparesARenamedFileWithWhereItWas()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"old.txt"_s, "one\ntwo\nthree\nfour\n");
        f.git({ u"add"_s, u"old.txt"_s });
        f.git({ u"commit"_s, u"-q"_s, u"-m"_s, u"First"_s });
        f.git({ u"mv"_s, u"old.txt"_s, u"new.txt"_s });
        f.write(u"new.txt"_s, "one\ntwo\nthree\nFour\n");
        f.git({ u"add"_s, u"new.txt"_s });

        auto diff = diffOf_(f, u"new.txt"_s);
        QVERIFY(diff);
        QCOMPARE(diff->hunks.size(), 1);

        // Only the changed line, not the whole file as removed and added
        const auto& lines = diff->hunks[0].lines;
        auto changed = std::count_if(
            lines.cbegin(),
            lines.cend(),
            [](const GitDiff::Line& line) {
                return line.kind != GitDiff::Line::Kind::Context;
            });
        QCOMPARE(changed, 2);
    }

    void binaryFileIsSaidToBe()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"map.png"_s, QByteArray("\x89PNG\0\x01\x02", 7));

        auto diff = diffOf_(f, u"map.png"_s);
        QVERIFY(diff);
        QVERIFY(diff->binary);
    }

    // Two asked for at once: only the second is answered
    void readDiffAskedAgainReplacesTheWaitingOne()
    {
        Fixture_ f{};
        f.git({ u"init"_s, u"-q"_s });
        f.write(u"a.txt"_s, "a\n");
        f.write(u"b.txt"_s, "b\n");

        f.repository.detect();
        QTRY_VERIFY(f.repository.isFound());
        f.repository.refresh();
        QTRY_VERIFY(entry_(f.repository, u"b.txt"_s));

        QStringList answered{};
        connect(
            &f.repository,
            &GitRepository::diffRead,
            this,
            [&answered](const Coco::Path& path, const GitDiff::Diff&) {
                answered << path.prettyQString();
            });

        // A status first, so both diffs wait behind it
        f.repository.refresh();
        f.repository.readDiff(*entry_(f.repository, u"a.txt"_s));
        f.repository.readDiff(*entry_(f.repository, u"b.txt"_s));

        QTRY_COMPARE(answered, QStringList{ u"b.txt"_s });
        QTest::qWait(QUIET_WAIT_MS_);
        QCOMPARE(answered, QStringList{ u"b.txt"_s });
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
