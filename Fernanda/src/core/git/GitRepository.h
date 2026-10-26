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
#include <optional>
#include <utility>

#include <QByteArray>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTypes>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/CoreConstants.h"
#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/git/GitDiff.h"
#include "core/git/GitStatus.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// The Git repository a vault's folder is in, as one vault window sees it.
// Finds the repository, reads its status, its diffs, and its branches,
// commits, discards a file's changes, switches and makes branches, and
// fetches, pulls, and pushes, by
// running the git installed on the computer, and announces each answer. A
// status covers the whole
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
// - --literal-pathspecs, so a path given to git names only itself: a file
//   named "[draft].md" isn't a pattern that also matches "d.md"
// - GIT_TERMINAL_PROMPT=0, and nothing on standard input beyond the paths a
//   command is given there, so a command that wants to ask something fails
//   instead of waiting forever for an answer no one can type
class GitRepository : public QObject
{
    Q_OBJECT

public:
    // What sync does, chosen as GitHub Desktop's one button chooses: by what
    // the branch needs
    enum class SyncAction
    {
        Fetch,
        Pull,
        Push,
        Publish
    };

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

        // The remotes, which a sync needs, are read with the status: they
        // change as rarely, and only outside Fernanda
        if (!isQueued_(Kind_::Remotes)) {
            enqueue_(
                Kind_::Remotes,
                root_,
                { u"config"_s, u"--get-regexp"_s, u"^remote\\..*\\.url$"_s });
        }
    }

    // Each remote's name and URL, from the repository's config, as last read
    // (refresh). remotesChanged says when they're read
    [[nodiscard]] const QMap<QString, QString>& remotes() const noexcept
    {
        return remotes_;
    }

    // The remote this branch syncs with: its upstream's, else origin, else
    // the only remote there is. Empty with none
    [[nodiscard]] QString syncRemote() const
    {
        if (auto remote = remoteOf(status_.branch.upstream);
            !remote.isEmpty()) {
            return remote;
        }

        if (remotes_.contains(u"origin"_s)) {
            return u"origin"_s;
        }

        return remotes_.size() == 1 ? remotes_.firstKey() : QString{};
    }

    // Which known remote a remote branch's name ("origin/draft") belongs to.
    // A remote's name can hold a slash, so the names are matched rather than
    // split, and the longest that matches wins: "a/b/main" is a/b's, not
    // a's, when both remotes exist. Empty when none matches
    [[nodiscard]] QString remoteOf(const QString& remoteBranch) const
    {
        QString found{};

        for (auto it = remotes_.cbegin(); it != remotes_.cend(); ++it) {
            if (it.key().size() > found.size() &&
                remoteBranch.startsWith(it.key() + u'/')) {
                found = it.key();
            }
        }

        return found;
    }

    // A remote's default branch ("main"), as its HEAD named it at the last
    // listBranches. Empty when unknown
    [[nodiscard]] QString defaultBranchOf(const QString& remote) const
    {
        return remoteDefaults_.value(remote);
    }

    // When this repository last fetched, in ms since the epoch: the time on
    // FETCH_HEAD, which every fetch writes, from Fernanda or not. 0 when it
    // never has
    [[nodiscard]] qint64 lastFetchedEpochMs() const
    {
        if (fetchHead_.isEmpty()) {
            return 0;
        }

        auto info = fetchHead_.toQFileInfo();
        return info.exists() ? info.lastModified().toMSecsSinceEpoch() : 0;
    }

    // The sync underway, if any
    [[nodiscard]] std::optional<SyncAction> syncing() const noexcept
    {
        if (operation_ != Operation_::Sync) {
            return std::nullopt;
        }

        return syncAction_;
    }

    // Whether a commit, a discard, a switch, or a sync is underway: from
    // asking until its answer (committed or commitFailed, discarded or
    // discardFailed, switched or switchFailed, synced or syncFailed). Each
    // waits for the others, so their steps can't interleave
    [[nodiscard]] bool isBusy() const noexcept
    {
        return operation_ != Operation_::None;
    }

    // Commit these files' changes, and nothing else, with summary as the
    // message's first line and description, if any, as its body after a blank
    // line. committed or commitFailed answers, and the status is read again
    // either way.
    //
    // Every modified buffer in the repository is saved first; if one can't
    // be, nothing is committed. A file git doesn't track yet is added first,
    // since a commit takes only files git knows. Then git commits only the
    // given paths' contents as they are on disk (commit --only), so what's
    // been staged for other files, by Fernanda or in a terminal, stays staged
    // and stays out of the commit. A renamed file's old path goes in too, so
    // its removal is committed with it. Git refuses this kind of commit
    // during a merge, and says so.
    //
    // The paths go to git on its standard input, so there's no limit on how
    // many
    void commit(
        const QList<GitStatus::Entry>& entries,
        const QString& summary,
        const QString& description)
    {
        if (!isFound() || isBusy() || entries.isEmpty()) {
            return;
        }

        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            emit commitFailed(
                tr("A file couldn't be saved, so nothing was committed."));
            return;
        }

        QByteArray untracked{};
        QByteArray paths{};

        for (const auto& entry : entries) {
            auto path = pathspecOf_(entry.path);

            if (entry.change == GitStatus::Change::Untracked) {
                untracked += path;
            }

            if (!entry.originalPath.isEmpty()) {
                paths += pathspecOf_(entry.originalPath);
            }

            paths += path;
        }

        operation_ = Operation_::Commit;

        Command_ command{};
        command.kind = Kind_::Commit;
        command.operation = Operation_::Commit;
        command.finishes = true;
        command.folder = root_;
        command.arguments = QStringList{ u"commit"_s,
                                         u"--quiet"_s,
                                         u"--only"_s,
                                         u"--message"_s,
                                         summary };

        if (!description.trimmed().isEmpty()) {
            command.arguments << u"--message"_s << description;
        }

        command.arguments << u"--pathspec-from-file=-"_s
                          << u"--pathspec-file-nul"_s;
        command.input = paths;

        if (!untracked.isEmpty()) {
            Command_ add{};
            add.kind = Kind_::Add;
            add.operation = Operation_::Commit;
            add.folder = root_;
            add.arguments = QStringList{ u"add"_s,
                                         u"--all"_s,
                                         u"--pathspec-from-file=-"_s,
                                         u"--pathspec-file-nul"_s };
            add.input = untracked;
            enqueue_(std::move(add));
        }

        enqueue_(std::move(command));
    }

    // Throw one file's changes away, returning it to the last commit, the
    // index included, as GitHub Desktop does. discarded or discardFailed
    // answers, and the status is read again either way. Every modified buffer
    // in the repository is saved first; if one can't be, nothing is
    // discarded. A conflicted file isn't discarded: that's a merge's to
    // settle.
    //
    // What's thrown away goes to the system trash first, and if it can't,
    // nothing is discarded, since git itself keeps no copy of changes never
    // committed:
    // - A file in the last commit is copied, under its own name, into a
    //   temporary folder, the copy goes to the trash, and git then restores
    //   the file in place (restore --source=HEAD --staged --worktree). The
    //   file is never missing, so a view open on it reloads it as one
    //   undoable change rather than closing. The trash records the temporary
    //   folder as where the copy came from, so putting it back puts it there.
    //   The folder is in Fernanda's own data folder, not the system's
    //   temporary one, which can be a separate disk (Linux's /tmp, often): a
    //   file trashed from another disk goes to that disk's own trash, which
    //   for /tmp is cleared when the computer restarts
    // - A file not in the last commit (untracked, newly added, or a rename's
    //   new path) goes to the trash itself, and leaves the index (rm
    //   --cached). A view open on it closes, as for any file deleted outside
    //   Fernanda
    //
    // A renamed file is both: its new path goes, and its old path is
    // restored. Before the first commit, every file is new
    void discard(const GitStatus::Entry& entry)
    {
        if (!isFound() || isBusy() ||
            entry.change == GitStatus::Change::Conflicted) {
            return;
        }

        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            emit discardFailed(
                tr("A file couldn't be saved, so nothing was discarded."));
            return;
        }

        // The path git restores from the last commit, and the path that goes
        Coco::Path restored{};
        Coco::Path removed{};

        if (status_.branch.unborn) {
            removed = entry.path;
        } else {
            switch (entry.change) {
            case GitStatus::Change::Modified:
            case GitStatus::Change::Deleted:
                restored = entry.path;
                break;
            case GitStatus::Change::Renamed:
                restored = entry.originalPath;
                removed = entry.path;
                break;
            case GitStatus::Change::Added:
            case GitStatus::Change::Untracked:
            case GitStatus::Change::Conflicted:
                removed = entry.path;
                break;
            }
        }

        if ((!restored.isEmpty() && !trashCopyOf_(root_ / restored)) ||
            (!removed.isEmpty() && !trash_(root_ / removed))) {
            emit discardFailed(
                tr("The changes couldn't be moved to the trash, so nothing "
                   "was discarded."));
            return;
        }

        QList<Command_> steps{};

        // An untracked file was never in the index
        if (!removed.isEmpty() &&
            entry.change != GitStatus::Change::Untracked) {
            Command_ remove{};
            remove.kind = Kind_::Remove;
            remove.arguments = QStringList{ u"rm"_s,
                                            u"--cached"_s,
                                            u"--quiet"_s,
                                            u"--ignore-unmatch"_s,
                                            u"--pathspec-from-file=-"_s,
                                            u"--pathspec-file-nul"_s };
            remove.input = pathspecOf_(removed);
            steps << remove;
        }

        if (!restored.isEmpty()) {
            Command_ restore{};
            restore.kind = Kind_::Restore;
            restore.arguments = QStringList{ u"restore"_s,
                                             u"--source=HEAD"_s,
                                             u"--staged"_s,
                                             u"--worktree"_s,
                                             u"--pathspec-from-file=-"_s,
                                             u"--pathspec-file-nul"_s };
            restore.input = pathspecOf_(restored);
            steps << restore;
        }

        operation_ = Operation_::Discard;

        if (steps.isEmpty()) {
            finish_(Operation_::Discard, {});
            return;
        }

        steps.last().finishes = true;

        for (auto& step : steps) {
            step.operation = Operation_::Discard;
            step.folder = root_;
            enqueue_(std::move(step));
        }
    }

    // Switch to another local branch (switch). switched or switchFailed
    // answers, and the status is read again either way. Refused, with no git
    // run, while a file git tracks has uncommitted changes, by the last
    // status read: they'd follow to the other branch, or stop the switch,
    // and either is a surprise. A file git doesn't track doesn't count, since
    // git leaves it where it is on every branch. Every modified buffer in the
    // repository is saved first, so the last status saw the text on screen.
    //
    // Files the switch changes on disk reload in their views as outside
    // changes do, and a view of a file the other branch doesn't have closes
    void switchBranch(const QString& name)
    {
        if (!isFound() || isBusy() || name.isEmpty()) {
            return;
        }

        if (auto refusal = changesRefusal_(); !refusal.isEmpty()) {
            emit switchFailed(refusal);
            return;
        }

        enqueueSwitch_({ u"switch"_s, u"--quiet"_s, u"--no-guess"_s, name });
    }

    // Switch to a remote's branch ("origin/draft"), making a local branch of
    // the same name that tracks it (switch --track), as GitHub Desktop does.
    // Answered and refused as switchBranch is
    void switchToRemoteBranch(const QString& remoteBranch)
    {
        if (!isFound() || isBusy() || remoteBranch.isEmpty()) {
            return;
        }

        if (auto refusal = changesRefusal_(); !refusal.isEmpty()) {
            emit switchFailed(refusal);
            return;
        }

        enqueueSwitch_(
            { u"switch"_s, u"--quiet"_s, u"--track"_s, remoteBranch });
    }

    // Talk to the branch's remote (syncRemote): fetch, pull, push, or
    // publish. synced or syncFailed answers, and the status is read again
    // either way, so the counts follow. Only ever run when asked: nothing
    // here fetches on its own.
    //
    // - Fetch: fetch --prune, so a branch deleted on the remote goes here too
    // - Pull: fast-forward only, without rebasing. Refused, as a switch is,
    //   while a tracked file has uncommitted changes. Branches that have
    //   diverged need a merge or a rebase, which this doesn't do; git says so
    // - Push: to the branch's upstream
    // - Publish: push --set-upstream, so the branch tracks its copy on the
    //   remote from then on. For a branch with no upstream, or one whose
    //   upstream is gone from the remote
    //
    // Git signs in its own way: a credential manager's window, an SSH agent.
    // A sync gets GIT_NETWORK_TIMEOUT_MS, and is stopped after
    void sync(SyncAction action)
    {
        if (!isFound() || isBusy()) {
            return;
        }

        auto remote = syncRemote();

        if (remote.isEmpty()) {
            emit syncFailed(tr("This repository has no remote."));
            return;
        }

        QStringList arguments{};

        switch (action) {
        case SyncAction::Fetch:
            arguments = { u"fetch"_s, u"--quiet"_s, u"--prune"_s, remote };
            break;
        case SyncAction::Pull:
            if (auto refusal = changesRefusal_(); !refusal.isEmpty()) {
                emit syncFailed(refusal);
                return;
            }

            arguments = { u"pull"_s,
                          u"--quiet"_s,
                          u"--no-rebase"_s,
                          u"--ff-only"_s };
            break;
        case SyncAction::Push:
            arguments = { u"push"_s, u"--quiet"_s };
            break;
        case SyncAction::Publish:
            arguments = { u"push"_s,
                          u"--quiet"_s,
                          u"--set-upstream"_s,
                          remote,
                          u"HEAD"_s };
            break;
        }

        operation_ = Operation_::Sync;
        syncAction_ = action;

        Command_ command{};
        command.kind = Kind_::Sync;
        command.operation = Operation_::Sync;
        command.finishes = true;
        command.folder = root_;
        command.arguments = arguments;
        command.timeoutMs = GIT_NETWORK_TIMEOUT_MS;
        enqueue_(std::move(command));
    }

    // Make a branch at the current commit and switch to it (switch
    // --create). switched or switchFailed answers, with git's own words for
    // a name it won't take. Uncommitted changes come along, as they do in
    // GitHub Desktop: the new branch starts where the current one is, so
    // nothing on disk changes
    void createBranch(const QString& name)
    {
        if (!isFound() || isBusy() || name.isEmpty()) {
            return;
        }

        enqueueSwitch_({ u"switch"_s, u"--quiet"_s, u"--create"_s, name });
    }

    // Read the branches' names, local ("draft") and remote ("origin/draft"),
    // each sorted as git sorts them, and each remote's default branch
    // (defaultBranchOf), from the HEAD a clone or a fetch left it.
    // branchesRead answers. A repository before its first commit has no local
    // branch, though it has a branch name (GitStatus::Branch::name). The
    // remote branches are as of the last fetch
    void listBranches()
    {
        if (!isFound() || isQueued_(Kind_::Branches)) {
            return;
        }

        enqueue_(
            Kind_::Branches,
            root_,
            { u"for-each-ref"_s,
              u"--format=%(refname)%00%(symref)"_s,
              u"refs/heads"_s,
              u"refs/remotes"_s });
    }

    // Read one changed file's diff against the last commit, which is what
    // committing it would record. diffRead or diffFailed answers, with the
    // entry's path. A file git doesn't track yet, and every file before the
    // first commit, is compared with nothing (diff --no-index against
    // /dev/null), so all of it shows as added. A renamed file is compared with
    // where it was. Asking again while one waits replaces the waiting one:
    // only the file asked for last is still wanted.
    //
    // External diff programs and text conversions are off. A repository's own
    // config can name either, and git would run it here; and the view wants
    // the file's own lines
    void readDiff(const GitStatus::Entry& entry)
    {
        if (!isFound()) {
            return;
        }

        queue_.removeIf([](const Command_& command) {
            return command.kind == Kind_::Diff;
        });

        Command_ command{};
        command.kind = Kind_::Diff;
        command.folder = root_;
        command.subject = entry.path;
        command.comparesWithNothing =
            entry.change == GitStatus::Change::Untracked ||
            status_.branch.unborn;

        command.arguments = QStringList{ u"diff"_s,
                                         u"--no-color"_s,
                                         u"--no-ext-diff"_s,
                                         u"--no-textconv"_s };

        if (command.comparesWithNothing) {
            command.arguments << u"--no-index"_s << u"--"_s << u"/dev/null"_s
                              << entry.path.prettyQString();
        } else {
            command.arguments << u"-M"_s << u"HEAD"_s << u"--"_s;

            if (!entry.originalPath.isEmpty()) {
                command.arguments << entry.originalPath.prettyQString();
            }

            command.arguments << entry.path.prettyQString();
        }

        enqueue_(std::move(command));
    }

    // Drop what's known: the repository, its status, and every command not
    // yet answered. A command already running finishes, and its answer is
    // ignored. foundChanged says so when a repository had been found
    void forget()
    {
        queue_.clear();
        ++generation_;
        operation_ = Operation_::None;

        if (root_.isEmpty()) {
            return;
        }

        root_ = {};
        status_ = {};
        remotes_ = {};
        remoteDefaults_ = {};
        fetchHead_ = {};

        emit foundChanged();
        emit statusChanged();
        emit remotesChanged();
    }

signals:
    // The vault's folder was found to be in a repository, in none, or in a
    // different one. The status is empty after
    void foundChanged();

    void statusChanged();

    // The diff readDiff asked for, of the file at path (relative to the
    // repository's folder)
    void diffRead(const Coco::Path& path, const Fernanda::GitDiff::Diff& diff);

    // readDiff's git failed, for the file at path. error is git's own message
    void diffFailed(const Coco::Path& path, const QString& error);

    // The commit asked for was made
    void committed();

    // The commit asked for wasn't made. error says why, most often in git's
    // own words
    void commitFailed(const QString& error);

    // The discard asked for was done
    void discarded();

    // The discard asked for wasn't done, or not wholly: what went to the
    // trash before a git step failed stays there. error says why
    void discardFailed(const QString& error);

    // A switch or a new branch asked for was made
    void switched();

    // A switch or a new branch asked for wasn't made. error says why
    void switchFailed(const QString& error);

    // The branches listBranches asked for
    void branchesRead(const QStringList& local, const QStringList& remote);

    // The remotes were read (remotes)
    void remotesChanged();

    // The sync asked for was done
    void synced();

    // The sync asked for wasn't done. error says why, most often in git's
    // own words
    void syncFailed(const QString& error);

private:
    // The folder, in Fernanda's data folder, that a discarded file's copy is
    // made in on its way to the trash (trashCopyOf_)
    static constexpr auto DISCARD_FOLDER_NAME_ = "Discarded";

    enum class Kind_
    {
        Detect,
        Status,
        Diff,
        Add,
        Commit,
        Remove,
        Restore,
        Switch,
        Branches,
        Remotes,
        GitPath,
        Sync
    };

    // What a command is a step of, beyond answering on its own
    enum class Operation_
    {
        None,
        Commit,
        Discard,
        Switch,
        Sync
    };

    struct Command_
    {
        Kind_ kind = Kind_::Detect;

        // The folder git runs in
        Coco::Path folder{};

        // Git's own arguments, after the ones every command gets
        QStringList arguments{};

        // A diff's file, relative to the repository's folder
        Coco::Path subject{};

        // A diff with --no-index, which exits with 1 when the files differ
        bool comparesWithNothing = false;

        // What git reads on its standard input: the paths, each ended by a
        // NUL, for a command given --pathspec-from-file=-. Empty for most
        QByteArray input{};

        // The operation it's a step of, and whether it's the last step
        Operation_ operation = Operation_::None;
        bool finishes = false;

        // generation_ when it was asked for. An answer from before the last
        // forget is ignored
        int generation = 0;

        // How long it may run before it's stopped. 0 is as long as it takes
        int timeoutMs = 0;
    };

    struct Result_
    {
        // Git ran and exited with 0
        bool succeeded = false;

        // What git exited with. -1 when it didn't start or didn't exit
        // normally
        int exitCode = -1;

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

    // The operation underway, if any, and for a sync, which
    Operation_ operation_ = Operation_::None;
    SyncAction syncAction_ = SyncAction::Fetch;

    // Stops a command that runs past its timeoutMs
    QTimer* timeout_ = new QTimer(this);
    bool timedOut_ = false;

    // remotes and defaultBranchOf, as last read
    QMap<QString, QString> remotes_{};
    QHash<QString, QString> remoteDefaults_{};

    // FETCH_HEAD, where the repository keeps it. Empty until read, after the
    // repository is found
    Coco::Path fetchHead_{};

    void setup_()
    {
        program_ = QStandardPaths::findExecutable(u"git"_s);

        if (program_.isEmpty()) {
            INFO("Git wasn't found");
            return;
        }

        environment_ = QProcessEnvironment::systemEnvironment();
        environment_.insert(u"GIT_TERMINAL_PROMPT"_s, u"0"_s);

        timeout_->setSingleShot(true);
        connect(timeout_, &QTimer::timeout, this, [this] {
            if (process_) {
                WARN(
                    "Stopping git, which ran past its time: {}",
                    process_->arguments().join(u' '));
                timedOut_ = true;
                process_->kill();
            }
        });
    }

    // Why a pull or a switch can't go ahead, or nothing. Every modified
    // buffer in the repository is saved first, so the last status saw the
    // text on screen. Then any change to a file git tracks refuses it: the
    // change would follow to the other branch, be mixed with what comes in,
    // or stop git partway, and each is a surprise. A file git doesn't track
    // doesn't count: git leaves it where it is
    [[nodiscard]] QString changesRefusal_()
    {
        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            return tr("A file couldn't be saved, so nothing was done.");
        }

        const auto& entries = status_.entries;
        auto has_changes = std::any_of(
            entries.cbegin(),
            entries.cend(),
            [](const GitStatus::Entry& entry) {
                return entry.change != GitStatus::Change::Untracked;
            });

        return has_changes ? tr("Commit or discard your changes first.")
                           : QString{};
    }

    // A path for --pathspec-from-file with --pathspec-file-nul: UTF-8,
    // ended by a NUL
    [[nodiscard]] static QByteArray pathspecOf_(const Coco::Path& path)
    {
        return path.prettyQString().toUtf8() + '\0';
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
        Command_ command{};
        command.kind = kind;
        command.folder = folder;
        command.arguments = arguments;
        enqueue_(std::move(command));
    }

    void enqueue_(Command_ command)
    {
        command.generation = generation_;
        queue_ << std::move(command);
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
            u"--literal-pathspecs"_s,
        };

        arguments << running_.arguments;

        process_ = new QProcess(this);
        process_->setProgram(program_);
        process_->setArguments(arguments);
        process_->setWorkingDirectory(running_.folder.toQString());
        process_->setProcessEnvironment(environment_);
        if (running_.input.isEmpty()) {
            process_->setStandardInputFile(QProcess::nullDevice());
        }

        connect(
            process_,
            &QProcess::finished,
            this,
            &GitRepository::onProcessFinished_);

        // Queued: on Windows a process that can't start reports it from
        // inside start(), and completing the command there would start the
        // next one under this function, which goes on to the input and the
        // timer below
        connect(
            process_,
            &QProcess::errorOccurred,
            this,
            &GitRepository::onProcessError_,
            Qt::QueuedConnection);

        process_->start();

        // Written once git has started, then closed, so git sees the end of
        // the paths
        if (!running_.input.isEmpty()) {
            process_->write(running_.input);
            process_->closeWriteChannel();
        }

        if (running_.timeoutMs > 0) {
            timeout_->start(running_.timeoutMs);
        }
    }

    void onProcessFinished_(int exitCode, QProcess::ExitStatus exitStatus)
    {
        Result_ result{};
        result.succeeded = exitStatus == QProcess::NormalExit && exitCode == 0;
        result.exitCode = exitStatus == QProcess::NormalExit ? exitCode : -1;
        result.output = process_->readAllStandardOutput();
        result.error =
            QString::fromUtf8(process_->readAllStandardError()).trimmed();

        // Some failures are told on standard output: "nothing to commit"
        if (!result.succeeded && result.error.isEmpty()) {
            result.error = QString::fromUtf8(result.output).trimmed();
        }

        if (!result.succeeded && result.error.isEmpty()) {
            result.error = process_->errorString();
        }

        // Stopped for running too long. A fetch stopped partway can leave a
        // lock file behind, which the next fetch will name
        if (timedOut_) {
            result.succeeded = false;
            result.error =
                tr("Git took longer than %Ln second(s), so it was stopped.",
                   nullptr,
                   running_.timeoutMs / 1000);
        }

        complete_(result);
    }

    // A process that never started reports nothing else, so this completes
    // its command. Every other error is followed by finished, which has
    // completed it by the time this queued call arrives. A process that
    // never started is still process_ here: nothing else completes it
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
        timeout_->stop();
        timedOut_ = false;

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
        case Kind_::Diff:
            onDiffRead_(command, result);
            break;
        case Kind_::Add:
        case Kind_::Commit:
        case Kind_::Remove:
        case Kind_::Restore:
        case Kind_::Switch:
        case Kind_::Sync:
            onStep_(command, result);
            break;
        case Kind_::Branches:
            onBranchesRead_(command, result);
            break;
        case Kind_::Remotes:
            onRemotesRead_(command, result);
            break;
        case Kind_::GitPath:
            onGitPathRead_(command, result);
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

        remotes_ = {};
        remoteDefaults_ = {};
        fetchHead_ = {};

        if (root_.isEmpty()) {
            INFO("No Git repository for {}", command.folder);
        } else {
            INFO("Git repository for {}: {}", command.folder, root_);

            // Where FETCH_HEAD is: in .git, or elsewhere for a worktree
            enqueue_(
                Kind_::GitPath,
                root_,
                { u"rev-parse"_s, u"--git-path"_s, u"FETCH_HEAD"_s });
        }

        emit foundChanged();
        emit statusChanged();
        emit remotesChanged();
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

    // A --no-index diff exits with 1 when the files differ, which is the
    // answer wanted; git's own errors exit with 1 too, but print no diff
    void onDiffRead_(const Command_& command, const Result_& result)
    {
        if (PathKey::of(command.folder) != PathKey::of(root_)) {
            return;
        }

        auto answered = result.succeeded ||
                        (command.comparesWithNothing && result.exitCode == 1 &&
                         !result.output.isEmpty());

        if (!answered) {
            WARN(
                "Git diff failed for {} in {}: {}",
                command.subject,
                command.folder,
                result.error);
            emit diffFailed(command.subject, result.error);
            return;
        }

        emit diffRead(command.subject, GitDiff::parse(result.output));
    }

    // One step of a commit. A failed step ends the commit, and the step after
    // it, already waiting, is dropped. The commit is the last step
    // A switch is one step, and the whole of its operation
    void enqueueSwitch_(const QStringList& arguments)
    {
        operation_ = Operation_::Switch;

        Command_ command{};
        command.kind = Kind_::Switch;
        command.operation = Operation_::Switch;
        command.finishes = true;
        command.folder = root_;
        command.arguments = arguments;
        enqueue_(std::move(command));
    }

    // Each line is a ref's full name, a NUL, and what it points to, if it's
    // a symbolic ref. A remote's HEAD is one, naming its default branch
    void onBranchesRead_(const Command_& command, const Result_& result)
    {
        if (PathKey::of(command.folder) != PathKey::of(root_)) {
            return;
        }

        if (!result.succeeded) {
            WARN(
                "Git couldn't list branches in {}: {}",
                command.folder,
                result.error);
            return;
        }

        static const auto heads = u"refs/heads/"_s;
        static const auto remotes = u"refs/remotes/"_s;

        QStringList local{};
        QStringList remote{};
        remoteDefaults_.clear();

        for (const auto& line : QString::fromUtf8(result.output).split(u'\n')) {
            auto fields = line.trimmed().split(QChar(u'\0'));
            const auto& ref = fields[0];
            auto target = fields.size() > 1 ? fields[1] : QString{};

            if (ref.startsWith(heads)) {
                local << ref.mid(heads.size());
                continue;
            }

            if (!ref.startsWith(remotes)) {
                continue;
            }

            auto name = ref.mid(remotes.size());

            if (!target.isEmpty()) {
                auto remote_name = remoteOf(name);
                auto prefix = remotes + remote_name + u'/';

                if (!remote_name.isEmpty() && target.startsWith(prefix)) {
                    remoteDefaults_.insert(
                        remote_name,
                        target.mid(prefix.size()));
                }

                continue;
            }

            remote << name;
        }

        emit branchesRead(local, remote);
    }

    // Lines of "remote.<name>.url <url>". git exits with 1, printing
    // nothing, when there are none
    void onRemotesRead_(const Command_& command, const Result_& result)
    {
        if (PathKey::of(command.folder) != PathKey::of(root_)) {
            return;
        }

        if (!result.succeeded && result.exitCode != 1) {
            WARN(
                "Git couldn't read the remotes in {}: {}",
                command.folder,
                result.error);
            return;
        }

        static const auto prefix = u"remote."_s;
        static const auto suffix = u".url"_s;

        QMap<QString, QString> read{};

        for (const auto& line : QString::fromUtf8(result.output).split(u'\n')) {
            auto space = line.indexOf(u' ');

            if (space < 0) {
                continue;
            }

            auto key = line.left(space);

            if (!key.startsWith(prefix) || !key.endsWith(suffix)) {
                continue;
            }

            auto name = key.mid(
                prefix.size(),
                key.size() - prefix.size() - suffix.size());

            // A remote may have several URLs; the first is the one fetched
            if (!name.isEmpty() && !read.contains(name)) {
                read.insert(name, line.mid(space + 1).trimmed());
            }
        }

        if (read != remotes_) {
            remotes_ = read;
            emit remotesChanged();
        }
    }

    // Relative to the repository's folder unless git gave it whole
    void onGitPathRead_(const Command_& command, const Result_& result)
    {
        if (PathKey::of(command.folder) != PathKey::of(root_) ||
            !result.succeeded) {
            return;
        }

        Coco::Path path = QString::fromUtf8(result.output).trimmed();
        fetchHead_ = path.toStd().is_absolute() ? path : root_ / path;
    }

    // One step of a commit, a discard, a switch, or a sync. A failed step ends
    // its operation, and the steps after it, already waiting, are dropped. The
    // last step's success ends it too
    void onStep_(const Command_& command, const Result_& result)
    {
        if (!result.succeeded) {
            queue_.removeIf([&command](const Command_& waiting) {
                return waiting.operation == command.operation;
            });

            WARN(
                "Git {} failed in {}: {}",
                command.arguments.first(),
                command.folder,
                result.error);
            finish_(command.operation, result.error);
            return;
        }

        if (command.finishes) {
            finish_(command.operation, {});
        }
    }

    // An empty error is success
    void finish_(Operation_ operation, const QString& error)
    {
        operation_ = Operation_::None;

        if (error.isEmpty()) {
            INFO("Git operation done in {}", root_);
        }

        switch (operation) {
        case Operation_::Commit:
            if (error.isEmpty()) {
                emit committed();
            } else {
                emit commitFailed(error);
            }
            break;
        case Operation_::Discard:
            if (error.isEmpty()) {
                emit discarded();
            } else {
                emit discardFailed(error);
            }
            break;
        case Operation_::Switch:
            if (error.isEmpty()) {
                emit switched();
            } else {
                emit switchFailed(error);
            }
            break;
        case Operation_::Sync:
            if (error.isEmpty()) {
                emit synced();
            } else {
                emit syncFailed(error);
            }
            break;
        case Operation_::None:
            break;
        }

        refresh();
    }

    // A copy of the file, under its own name, to the trash, by way of a
    // temporary folder in Fernanda's data folder (discard). A file that isn't
    // there has nothing to keep
    [[nodiscard]] static bool trashCopyOf_(const Coco::Path& file)
    {
        if (!file.isFile()) {
            return true;
        }

        auto parent = Coco::Path::AppLocalData(DISCARD_FOLDER_NAME_);

        if (!Coco::mkpath(parent)) {
            WARN(
                "Couldn't make {}, to copy {} into before a discard",
                parent,
                file);
            return false;
        }

        QTemporaryDir folder((parent / u"XXXXXX"_s).toQString());
        auto copy = Coco::Path(folder.path()) / file.name();

        if (!folder.isValid() || !Coco::copy(file, copy) ||
            !Coco::moveToTrash(copy)) {
            WARN("Couldn't move a copy of {} to the trash", file);
            return false;
        }

        return true;
    }

    // The file itself to the trash. A file that isn't there has nothing to
    // keep
    [[nodiscard]] static bool trash_(const Coco::Path& file)
    {
        if (!file.exists()) {
            return true;
        }

        if (!Coco::moveToTrash(file)) {
            WARN("Couldn't move {} to the trash", file);
            return false;
        }

        return true;
    }
};

} // namespace Fernanda
