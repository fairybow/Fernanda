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

#pragma once

#include <algorithm>
#include <utility>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/CoreConstants.h"
#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/git/GitStatus.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// The Git repository a vault's folder is in, as one vault window sees it.
// Finds the repository and reads its status by running the git installed on
// the computer, and announces each answer. A status covers the whole
// repository, which may hold more than the vault: other vaults, or files that
// aren't Fernanda's at all.
//
// Owned by its VaultWindow, which decides when to ask. Nothing here runs git
// on its own, so git runs only when the window asks. Two windows over one
// repository have one of these each; the repository itself is the shared
// state, and each reads it when it needs to.
//
// Commands run one at a time, in the order asked. Git takes a lock on the
// repository's index for most commands, so two at once could fail each
// other. Asking for a detection or a status while the same is already
// waiting adds nothing: the waiting one will read the newer state.
//
// Every command runs with:
// - core.fsmonitor off. A repository's own config can name a program for Git
//   to run as its file monitor, and status runs it. Fernanda asks for a
//   status as soon as a vault opens, and a vault can come from anyone, so
//   that program must never run unasked
// - core.quotePath off, so paths in Git's output are plain UTF-8
// - GIT_TERMINAL_PROMPT=0 and an empty standard input, so a command that
//   wants to ask something fails instead of waiting forever for an answer
//   no one can type
class GitRepository : public QObject
{
    Q_OBJECT

public:
    // bufferStore is App's and vault is the window's; both outlive this
    GitRepository(
        Vault* vault,
        BufferStore* bufferStore,
        QObject* parentVaultWindow)
        : QObject(parentVaultWindow)
        , vault_(vault)
        , bufferStore_(bufferStore)
    {
        setup_();
    }

    // A command still running is given GIT_EXIT_WAIT_MS to finish before it
    // is stopped. Stopping git partway can leave its lock file behind, which
    // blocks the next command until the file is removed by hand
    ~GitRepository() override
    {
        TRACER;
        queue_.clear();

        if (!process_) {
            return;
        }

        process_->disconnect(this);

        if (!process_->waitForFinished(GIT_EXIT_WAIT_MS)) {
            WARN(
                "Stopping git, which didn't finish in time: {}",
                process_->arguments().join(u' '));
            process_->kill();
            process_->waitForFinished(GIT_EXIT_WAIT_MS);
        }
    }

    // Whether git was found on the computer
    [[nodiscard]] bool hasGit() const noexcept { return !program_.isEmpty(); }

    // Whether the vault's folder is in a repository, as of the last detection
    [[nodiscard]] bool isFound() const noexcept { return !root_.isEmpty(); }

    // The repository's folder. Empty when none was found
    [[nodiscard]] Coco::Path root() const noexcept { return root_; }

    // The last status read. Empty until the first refresh, and again when
    // the repository is lost
    [[nodiscard]] const GitStatus::Status& status() const noexcept
    {
        return status_;
    }

    // Find the repository the vault's folder is in. Git is asked from the
    // folder, so the answer follows Git's own rules: it walks up to find a
    // repository, follows a worktree's or submodule's .git file, and refuses
    // a folder it doesn't trust. foundChanged says when the answer differs
    // from the last one.
    //
    // Git is asked how far up the repository's folder is (rev-parse
    // --show-cdup), not where it is (--show-toplevel), and the root is made
    // by walking up from the vault's folder. Git reports a folder with every
    // symlink above it resolved, as macOS's temporary folders are, and a key
    // made from that spelling wouldn't contain the keys the store's buffers
    // are opened by, so refresh would save none of them. Walked up from the
    // vault's folder, the root is spelled as everything else in the vault is
    void detect()
    {
        if (!hasGit() || isQueued_(Kind_::Detect)) {
            return;
        }

        enqueue_(
            Kind_::Detect,
            vault_->root(),
            { u"rev-parse"_s, u"--show-cdup"_s });
    }

    // Read the branch and every changed file. Every modified buffer in the
    // repository is saved first, so the status sees the text on screen and
    // not what autosave last wrote
    void refresh()
    {
        if (!isFound() || isQueued_(Kind_::Status)) {
            return;
        }

        // A failed save is logged by the store. The status still runs, and
        // reports the file as it is on disk
        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            WARN("Reading Git status with an unsaved file in {}", root_);
        }

        // --no-optional-locks: a status Fernanda reads for itself shouldn't
        // take the index lock, which a git run elsewhere at the same moment
        // would then fail on
        enqueue_(
            Kind_::Status,
            root_,
            { u"--no-optional-locks"_s,
              u"status"_s,
              u"--porcelain=v2"_s,
              u"--branch"_s,
              u"-z"_s,
              u"--untracked-files=all"_s });
    }

    // Drop what's known: the repository, its status, and every command not
    // yet answered. A command already running finishes, and its answer is
    // ignored. foundChanged says so when a repository had been found
    void forget()
    {
        queue_.clear();
        ++generation_;

        if (root_.isEmpty()) {
            return;
        }

        root_ = {};
        status_ = {};

        emit foundChanged();
        emit statusChanged();
    }

signals:
    // The vault's folder was found to be in a repository, in none, or in a
    // different one. The status is empty after
    void foundChanged();

    void statusChanged();

private:
    enum class Kind_
    {
        Detect,
        Status
    };

    struct Command_
    {
        Kind_ kind = Kind_::Detect;

        // The folder git runs in
        Coco::Path folder{};

        // Git's own arguments, after the ones every command gets
        QStringList arguments{};

        // generation_ when it was asked for. An answer from before the last
        // forget is ignored
        int generation = 0;
    };

    struct Result_
    {
        // Git ran and exited with 0
        bool succeeded = false;

        QByteArray output{};

        // Git's error output, or why it couldn't run
        QString error{};
    };

    Vault* vault_;
    BufferStore* bufferStore_;

    // Where git is. Empty when it wasn't found
    QString program_{};
    QProcessEnvironment environment_{};

    Coco::Path root_{};
    GitStatus::Status status_{};

    QList<Command_> queue_{};
    QProcess* process_ = nullptr;
    Command_ running_{};

    // Counts forgets (forget)
    int generation_ = 0;

    void setup_()
    {
        program_ = QStandardPaths::findExecutable(u"git"_s);

        if (program_.isEmpty()) {
            INFO("Git wasn't found");
            return;
        }

        environment_ = QProcessEnvironment::systemEnvironment();
        environment_.insert(u"GIT_TERMINAL_PROMPT"_s, u"0"_s);
    }

    [[nodiscard]] bool isQueued_(Kind_ kind) const
    {
        return std::any_of(
            queue_.cbegin(),
            queue_.cend(),
            [kind](const Command_& command) { return command.kind == kind; });
    }

    void
    enqueue_(Kind_ kind, const Coco::Path& folder, const QStringList& arguments)
    {
        queue_ << Command_{ kind, folder, arguments, generation_ };
        runNext_();
    }

    void runNext_()
    {
        if (process_ || queue_.isEmpty()) {
            return;
        }

        running_ = queue_.takeFirst();

        QStringList arguments{
            u"-c"_s,
            u"core.fsmonitor=false"_s,
            u"-c"_s,
            u"core.quotePath=false"_s,
        };

        arguments << running_.arguments;

        process_ = new QProcess(this);
        process_->setProgram(program_);
        process_->setArguments(arguments);
        process_->setWorkingDirectory(running_.folder.toQString());
        process_->setProcessEnvironment(environment_);
        process_->setStandardInputFile(QProcess::nullDevice());

        connect(
            process_,
            &QProcess::finished,
            this,
            &GitRepository::onProcessFinished_);

        connect(
            process_,
            &QProcess::errorOccurred,
            this,
            &GitRepository::onProcessError_);

        process_->start();
    }

    void onProcessFinished_(int exitCode, QProcess::ExitStatus exitStatus)
    {
        Result_ result{};
        result.succeeded = exitStatus == QProcess::NormalExit && exitCode == 0;
        result.output = process_->readAllStandardOutput();
        result.error =
            QString::fromUtf8(process_->readAllStandardError()).trimmed();

        if (!result.succeeded && result.error.isEmpty()) {
            result.error = process_->errorString();
        }

        complete_(result);
    }

    // A process that never started reports nothing else, so this completes
    // its command. Every other error is followed by finished
    void onProcessError_(QProcess::ProcessError error)
    {
        if (error != QProcess::FailedToStart) {
            return;
        }

        Result_ result{};
        result.error = process_->errorString();
        complete_(result);
    }

    void complete_(const Result_& result)
    {
        auto* process = process_;
        process_ = nullptr;
        process->deleteLater();

        auto command = std::move(running_);
        running_ = {};

        if (command.generation != generation_) {
            runNext_();
            return;
        }

        switch (command.kind) {
        case Kind_::Detect:
            onDetected_(command, result);
            break;
        case Kind_::Status:
            onStatusRead_(command, result);
            break;
        }

        runNext_();
    }

    void onDetected_(const Command_& command, const Result_& result)
    {
        Coco::Path found{};

        // Empty output means the vault's folder is the repository's. The
        // key removes the ".." steps
        if (result.succeeded) {
            auto up = QString::fromUtf8(result.output).trimmed();
            found = PathKey::of(command.folder / Coco::Path(up)).path();
        } else {
            // Most often the folder simply isn't in a repository
            DEBUG("No Git repository for {}: {}", command.folder, result.error);
        }

        if (PathKey::of(found) == PathKey::of(root_)) {
            return;
        }

        root_ = found;
        status_ = {};

        if (root_.isEmpty()) {
            INFO("No Git repository for {}", command.folder);
        } else {
            INFO("Git repository for {}: {}", command.folder, root_);
        }

        emit foundChanged();
        emit statusChanged();
    }

    void onStatusRead_(const Command_& command, const Result_& result)
    {
        // Read for a repository that has since been lost or replaced
        if (PathKey::of(command.folder) != PathKey::of(root_)) {
            return;
        }

        if (!result.succeeded) {
            WARN("Git status failed in {}: {}", command.folder, result.error);
            return;
        }

        status_ = GitStatus::parse(result.output);
        emit statusChanged();
    }
};

} // namespace Fernanda
