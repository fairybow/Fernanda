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

#pragma once

#include <algorithm>
#include <optional>
#include <utility>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTypes>

#ifdef Q_OS_UNIX
#    include <unistd.h>
#endif

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/BufferStore.h"
#include "core/CoreConstants.h"
#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/git/GitDiff.h"
#include "core/git/GitSafeDirectory.h"
#include "core/git/GitStatus.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// The Git repository a vault's folder is in, as one vault window sees it.
// Finds the repository, reads its status, its diffs, and its branches,
// commits, discards a file's changes, switches and makes branches, fetches,
// pulls, and pushes, and trusts a repository git refuses, by running the git
// installed on the computer, and announces each answer. A status covers the
// whole repository, which may hold more than the vault: other vaults, or
// files that aren't Fernanda's at all.
//
// Owned by its VaultWindow, which decides when to ask. Nothing here runs git
// unasked: each command is one the window asked for, a step of an operation
// it asked for, the status read that ends an operation, or the detection
// that follows trusting. Two windows over one repository have one of these
// each; the repository itself is the shared state, and each reads it when
// it needs to.
//
// Commands run one at a time, in the order asked. Git takes a lock on the
// repository's index for most commands, so two at once could fail each
// other. Asking for a detection, a status, a branch list, or the remotes
// while the same is already waiting adds nothing: the waiting one will read
// the newer state. Asking for a file's diff while one of the same file waits
// replaces it.
//
// Every command runs with:
// - core.fsmonitor off. A repository's own config can name a program for Git
//   to run as its file monitor, and status runs it. Fernanda asks for a
//   status as soon as a vault opens, and a vault can come from anyone, so
//   that program must never run unasked. Other programs a repository's config
//   can name still run, as they do for any Git client: a clean filter, when
//   a status or a diff reads a changed file, and hooks, when a commit or a
//   switch runs them
// - core.quotePath off, so paths in Git's output are plain UTF-8
// - --literal-pathspecs, so a path given to git names only itself: a file
//   named "[draft].md" isn't a pattern that also matches "d.md"
// - GIT_TERMINAL_PROMPT=0, nothing on standard input beyond the paths a
//   command is given there, and, on macOS and Linux, no terminal: git runs
//   in a session of its own (setsid). A command that wants to ask something
//   fails instead of waiting forever for an answer no one can type. Without
//   the last, ssh asks for a key's passphrase on the terminal Fernanda was
//   started from, where no one is looking
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
    // is stopped, and a step that rewrites files GIT_REWRITE_EXIT_WAIT_MS.
    // Stopping git partway can leave its lock file behind, which blocks the
    // next command until the file is removed by hand, and a step that
    // rewrites files would leave them half updated.
    //
    // A step that rewrites files has its hold on the store's autosave
    // released (holdWritesAtOrUnder), on the next turn of the event loop:
    // the store outlives this, but the views its release may reload belong
    // to a window being taken down now
    ~GitRepository() override
    {
        TRACER;
        queue_.clear();

        if (!process_) {
            return;
        }

        process_->disconnect(this);

        auto wait_ms = running_.rewritesFiles ? GIT_REWRITE_EXIT_WAIT_MS
                                              : GIT_EXIT_WAIT_MS;

        if (!process_->waitForFinished(wait_ms)) {
            WARN(
                "Stopping git, which didn't finish in time: {}",
                process_->arguments().join(u' '));
            process_->kill();
            process_->waitForFinished(GIT_EXIT_WAIT_MS);
        }

        if (running_.rewritesFiles) {
            QMetaObject::invokeMethod(
                bufferStore_,
                [store = bufferStore_, key = PathKey::of(running_.folder)] {
                    store->releaseWritesAtOrUnder(key);
                },
                Qt::QueuedConnection);
        }
    }

    // Whether the vault's folder is in a repository, as of the last detection
    [[nodiscard]] bool isFound() const noexcept { return !root_.isEmpty(); }

    // The repository's folder. Empty when none was found
    [[nodiscard]] Coco::Path root() const noexcept { return root_; }

    // The folder of a repository git found but refuses to trust, as the
    // last detection saw it: owned by another user on the computer, as one
    // on an exFAT drive or a network share often is (GitSafeDirectory).
    // Spelled as git wants it added to safe.directory. Empty otherwise, and
    // whenever a repository is found
    [[nodiscard]] QString untrustedFolder() const { return untrusted_; }

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
        if (!hasGit_() || isQueued_(Kind_::Detect)) {
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

        enqueue_(Kind_::Status, root_, statusArguments_());

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
    // A remote's name can hold a slash (newer git refuses to add one nested
    // in another's name, but a repository made earlier can have both), so the
    // names are matched rather than split, and the longest that matches wins:
    // "a/b/main" is a/b's, not a's, when both remotes exist. Empty when none
    // matches
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
        return fetchHead_.isEmpty() ? 0 : fetchHead_.lastModifiedEpochMs();
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
    // waits for the others, so their steps can't interleave. busyChanged
    // says when this changes
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

        setOperation_(Operation_::Commit);

        Command_ command{};
        command.kind = Kind_::Step;
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
            add.kind = Kind_::Step;
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
            remove.kind = Kind_::Step;
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
            restore.kind = Kind_::Step;
            restore.arguments = QStringList{ u"restore"_s,
                                             u"--source=HEAD"_s,
                                             u"--staged"_s,
                                             u"--worktree"_s,
                                             u"--pathspec-from-file=-"_s,
                                             u"--pathspec-file-nul"_s };
            restore.input = pathspecOf_(restored);
            restore.rewritesFiles = true;
            steps << restore;
        }

        setOperation_(Operation_::Discard);

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
    // answers every call, and the status is read again after a switch git
    // ran, whatever its answer. Every modified buffer in the repository is
    // saved, and then the status is read, before git switches: while a file
    // git tracks has uncommitted changes, the switch is refused
    // (enqueueChangesCheck_).
    //
    // Files the switch changes on disk reload in their views as outside
    // changes do, and a view of a file the other branch doesn't have closes
    void switchBranch(const QString& name)
    {
        if (auto refusal = switchRefusal_(name); !refusal.isEmpty()) {
            emit switchFailed(refusal);
            return;
        }

        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            emit switchFailed(
                tr("A file couldn't be saved, so nothing was done."));
            return;
        }

        enqueueSwitch_(
            { u"switch"_s, u"--quiet"_s, u"--no-guess"_s, name },
            true);
    }

    // Switch to a remote's branch ("origin/draft"), making a local branch of
    // the same name that tracks it (switch --track), as GitHub Desktop does.
    // Answered and refused as switchBranch is
    void switchToRemoteBranch(const QString& remoteBranch)
    {
        if (auto refusal = switchRefusal_(remoteBranch); !refusal.isEmpty()) {
            emit switchFailed(refusal);
            return;
        }

        if (!bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            emit switchFailed(
                tr("A file couldn't be saved, so nothing was done."));
            return;
        }

        enqueueSwitch_(
            { u"switch"_s, u"--quiet"_s, u"--track"_s, remoteBranch },
            true);
    }

    // Talk to the branch's remote (syncRemote): fetch, pull, push, or
    // publish. synced or syncFailed answers, and the status is read again
    // either way, so the counts follow. Only ever run when asked: nothing
    // here fetches on its own.
    //
    // - Fetch: fetch --prune, so a branch deleted on the remote goes here too
    // - Pull: what git pull does, in two steps: a fetch, then merge
    //   --ff-only with the upstream, which only fast-forwards. Only the fetch
    //   reaches the network, so only it can be stopped for taking too long;
    //   the merge, which rewrites files, is never stopped partway. Every
    //   modified buffer in the repository is saved first. Uncommitted changes
    //   stay as they are, as in GitHub Desktop: git takes in the commits only
    //   when none touches a file with changes, or an untracked file in the
    //   way, and otherwise stops before changing anything and names the
    //   files. Branches that have diverged need a merge or a rebase, which
    //   this doesn't do; git says so
    // - Push: to the branch's upstream
    // - Publish: push --set-upstream, so the branch tracks its copy on the
    //   remote from then on. For a branch with no upstream, or one whose
    //   upstream is gone from the remote
    //
    // Git signs in its own way: a credential manager's window, an SSH agent.
    // A fetch or a push gets GIT_NETWORK_TIMEOUT_MS, and is stopped after
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

        if (action == SyncAction::Pull &&
            !bufferStore_->flushAtOrUnder(PathKey::of(root_))) {
            emit syncFailed(
                tr("A file couldn't be saved, so nothing was done."));
            return;
        }

        // What reaches the network: the whole of a fetch, a push, or a
        // publish, and the first step of a pull
        QStringList network{};

        switch (action) {
        case SyncAction::Fetch:
        case SyncAction::Pull:
            network = { u"fetch"_s, u"--quiet"_s, u"--prune"_s, remote };
            break;
        case SyncAction::Push:
            network = { u"push"_s, u"--quiet"_s };
            break;
        case SyncAction::Publish:
            network = { u"push"_s,
                        u"--quiet"_s,
                        u"--set-upstream"_s,
                        remote,
                        u"HEAD"_s };
            break;
        }

        syncAction_ = action;
        setOperation_(Operation_::Sync);

        Command_ command{};
        command.kind = Kind_::Step;
        command.operation = Operation_::Sync;
        command.finishes = action != SyncAction::Pull;
        command.folder = root_;
        command.arguments = network;
        command.timeoutMs = GIT_NETWORK_TIMEOUT_MS;
        enqueue_(std::move(command));

        if (action != SyncAction::Pull) {
            return;
        }

        // The upstream as it is after the fetch. Nothing here reaches the
        // network, so nothing stops it partway
        Command_ merge{};
        merge.kind = Kind_::Step;
        merge.operation = Operation_::Sync;
        merge.finishes = true;
        merge.folder = root_;
        merge.arguments = QStringList{ u"merge"_s,
                                       u"--quiet"_s,
                                       u"--ff-only"_s,
                                       u"@{upstream}"_s };
        merge.rewritesFiles = true;
        enqueue_(std::move(merge));
    }

    // Make a branch at the current commit and switch to it (switch
    // --create). switched or switchFailed answers every call, with git's own
    // words for a name it won't take. Uncommitted changes come along, as they
    // do in GitHub Desktop: the new branch starts where the current one is, so
    // nothing on disk changes
    void createBranch(const QString& name)
    {
        if (auto refusal = switchRefusal_(name); !refusal.isEmpty()) {
            emit switchFailed(refusal);
            return;
        }

        enqueueSwitch_(
            { u"switch"_s, u"--quiet"_s, u"--create"_s, name },
            false);
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
    // where it was. Asking again for a file whose diff waits replaces the
    // waiting one, which would read the same file; diffs of different files
    // are each answered, since several diffs can be open at once.
    //
    // External diff programs and text conversions are off. A repository's own
    // config can name either, and git would run it here; and the view wants
    // the file's own lines
    void readDiff(const GitStatus::Entry& entry)
    {
        if (!isFound()) {
            return;
        }

        queue_.removeIf([&entry](const Command_& command) {
            return command.kind == Kind_::Diff && command.subject == entry.path;
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

    // Trust the folder git refused (untrustedFolder), as GitHub Desktop's
    // Trust repository does: add it to safe.directory in the user's own Git
    // config (config --global), then look for the repository again. The only
    // command here that writes outside the repository, so it runs only when
    // asked. trustFailed answers a failure
    void trust()
    {
        if (!hasGit_() || untrusted_.isEmpty() || isQueued_(Kind_::Trust)) {
            return;
        }

        enqueue_(
            Kind_::Trust,
            vault_->root(),
            { u"config"_s,
              u"--global"_s,
              u"--add"_s,
              u"safe.directory"_s,
              untrusted_ });
    }

    // Drop what's known: the repository, its status, and every command not
    // yet answered. A command already running finishes, and its answer is
    // ignored. foundChanged says so when a repository had been found, or
    // refused
    void forget()
    {
        queue_.clear();
        ++generation_;
        setOperation_(Operation_::None);

        trustChecking_ = false;

        if (root_.isEmpty() && untrusted_.isEmpty()) {
            return;
        }

        untrusted_ = {};
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
    // different one, or in one git refuses to trust (untrustedFolder). The
    // status is empty after
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

    // isBusy changed: an operation started, or the one underway ended. It
    // ends just before its answer (committed, commitFailed, and the rest)
    void busyChanged();

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

    // trust couldn't add the folder to the user's Git config. error is git's
    void trustFailed(const QString& error);

private:
    // The folder, in Fernanda's data folder, that a discarded file's copy is
    // made in on its way to the trash (trashCopyOf_)
    static constexpr auto DISCARD_FOLDER_NAME_ = "Discarded";

    // What a command answers. A step of an operation is answered by its
    // operation (Command_::operation), whatever git command it runs
    enum class Kind_
    {
        Detect,
        Status,
        Diff,
        Branches,
        Remotes,
        GitPath,
        Trust,
        Step
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

        // It may rewrite files in the working tree (a switch, a pull's merge,
        // a discard's restore). Everything open in the repository is saved
        // just before it starts, so git sees the text on screen and refuses
        // rather than overwrite a change it would touch, and the store holds
        // its autosave until it ends (BufferStore::holdWritesAtOrUnder)
        bool rewritesFiles = false;
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

    [[nodiscard]] bool hasGit_() const noexcept { return !program_.isEmpty(); }

    Coco::Path root_{};
    QString untrusted_{};

    // A trust was written to the config, and the detection after it hasn't
    // answered yet (onTrusted_)
    bool trustChecking_ = false;
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

    // Why a switch or a new branch can't be asked for at all, or nothing
    [[nodiscard]] QString switchRefusal_(const QString& name) const
    {
        if (!isFound()) {
            return tr("No Git repository was found.");
        }

        if (isBusy()) {
            return tr("Git is still busy with something else.");
        }

        if (name.isEmpty()) {
            return tr("No branch was named.");
        }

        return {};
    }

    // What every status read asks for. --no-optional-locks: a status
    // Fernanda reads for itself shouldn't take the index lock, which a git run
    // elsewhere at the same moment would then fail on
    [[nodiscard]] static QStringList statusArguments_()
    {
        return { u"--no-optional-locks"_s,
                 u"status"_s,
                 u"--porcelain=v2"_s,
                 u"--branch"_s,
                 u"-z"_s,
                 u"--untracked-files=all"_s };
    }

    // The first step of a switch: a status read, made after the caller has
    // saved every modified buffer in the repository, so it sees the text on
    // screen. onStatusRead_ lets the steps after it run only while no file
    // git tracks has uncommitted changes: they'd follow to the other branch,
    // or stop git partway, and either is a surprise. A file git doesn't track
    // doesn't count: git leaves it where it is.
    //
    // TODO: Offer GitHub Desktop's choice instead, leaving the changes on
    // this branch (stashed) or bringing them to the other
    void enqueueChangesCheck_(Operation_ operation)
    {
        Command_ check{};
        check.kind = Kind_::Status;
        check.operation = operation;
        check.folder = root_;
        check.arguments = statusArguments_();
        enqueue_(std::move(check));
    }

    // Whether the last status has a change to a file git tracks
    [[nodiscard]] bool hasTrackedChanges_() const
    {
        const auto& entries = status_.entries;

        return std::any_of(
            entries.cbegin(),
            entries.cend(),
            [](const GitStatus::Entry& entry) {
                return entry.change != GitStatus::Change::Untracked;
            });
    }

    // The steps of operation still waiting in the queue, dropped when an
    // earlier one ends it
    void dropSteps_(Operation_ operation)
    {
        queue_.removeIf([operation](const Command_& waiting) {
            return waiting.operation == operation;
        });
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

        // A failed save is logged by the store. The step still runs: the
        // buffer keeps its text, and if git rewrites the file, the hold's
        // release adopts git's as an undoable change
        if (running_.rewritesFiles) {
            auto key = PathKey::of(running_.folder);
            [[maybe_unused]] auto saved = bufferStore_->flushAtOrUnder(key);
            bufferStore_->holdWritesAtOrUnder(key);
        }

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

#ifdef Q_OS_UNIX
        // In the child, before git starts: a session of its own has no
        // terminal, so nothing git runs (ssh, a credential helper) can ask
        // on the one Fernanda was started from
        process_->setChildProcessModifier([] { ::setsid(); });
#endif

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

        // Held by QProcess until git has started, then the channel closed, so
        // git sees the end of the paths. A git that couldn't start (reported
        // from start() itself on Windows) has no channel to write to
        if (!running_.input.isEmpty() &&
            process_->state() != QProcess::NotRunning) {
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

        auto command = std::move(running_);
        running_ = {};

        // Released before the answer, so the buffers have taken in what git
        // wrote by the time anything hears of it
        if (command.rewritesFiles) {
            bufferStore_->releaseWritesAtOrUnder(PathKey::of(command.folder));
        }

        // process_ stays set while the answer is handled, so a command asked
        // for from a slot of a signal emitted here (a diff tab reading its
        // diff on statusChanged, say) waits in the queue rather than starting
        // at once. Started now, it could be the next step of an operation the
        // answer is still deciding to drop: a switch whose changes check is
        // about to refuse it would run anyway
        if (command.generation == generation_) {
            answer_(command, result);
        }

        process_->deleteLater();
        process_ = nullptr;
        runNext_();
    }

    void answer_(const Command_& command, const Result_& result)
    {
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
        case Kind_::Step:
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
        case Kind_::Trust:
            onTrusted_(result);
            break;
        }
    }

    void onDetected_(const Command_& command, const Result_& result)
    {
        Coco::Path found{};
        QString untrusted{};

        // Empty output means the vault's folder is the repository's. The
        // key removes the ".." steps
        if (result.succeeded) {
            auto up = QString::fromUtf8(result.output).trimmed();
            found = PathKey::of(command.folder / Coco::Path(up)).path();
        } else {
            // Most often the folder simply isn't in a repository
            DEBUG("No Git repository for {}: {}", command.folder, result.error);
            untrusted = GitSafeDirectory::folderOf(result.error);
        }

        // The folder was added to safe.directory, and git still refuses it:
        // the spelling git asked for didn't match, or another folder above
        // it is refused too
        if (std::exchange(trustChecking_, false) && !untrusted.isEmpty()) {
            WARN("Git still doesn't trust {} after trusting it", untrusted);
            emit trustFailed(tr("Git still doesn't trust this folder."));
        }

        if (PathKey::of(found) == PathKey::of(root_) &&
            untrusted == untrusted_) {
            return;
        }

        untrusted_ = untrusted;
        root_ = found;
        status_ = {};

        remotes_ = {};
        remoteDefaults_ = {};
        fetchHead_ = {};

        if (!untrusted_.isEmpty()) {
            INFO(
                "Git doesn't trust {}, which holds {}",
                untrusted_,
                command.folder);
        } else if (root_.isEmpty()) {
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

    // The folder is in the user's Git config now, or git said why not.
    // Detection runs again either way, and finds the repository once git
    // trusts it. If git still refuses it after the config took it,
    // onDetected_ says so (trustChecking_)
    void onTrusted_(const Result_& result)
    {
        if (!result.succeeded) {
            WARN("Git couldn't trust {}: {}", untrusted_, result.error);
            emit trustFailed(result.error);
        } else {
            trustChecking_ = true;
        }

        detect();
    }

    // A status read on its own, or as the first step of a switch
    // (enqueueChangesCheck_). A status read for a repository that has since
    // been lost or replaced is ignored. A first step that can't decide (that
    // repository, or a failed read) ends its operation as a refusal does
    void onStatusRead_(const Command_& command, const Result_& result)
    {
        auto current = PathKey::of(command.folder) == PathKey::of(root_);

        if (current && result.succeeded) {
            status_ = GitStatus::parse(result.output);
            emit statusChanged();
        } else if (current) {
            WARN("Git status failed in {}: {}", command.folder, result.error);
        }

        if (command.operation == Operation_::None) {
            return;
        }

        QString refusal{};

        if (!current) {
            refusal = tr("The repository changed, so nothing was done.");
        } else if (!result.succeeded) {
            refusal = result.error;
        } else if (hasTrackedChanges_()) {
            refusal = tr("Commit or discard your changes first.");
        }

        if (!refusal.isEmpty()) {
            dropSteps_(command.operation);
            finish_(command.operation, refusal);
        }
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

    // A switch is one git step, after the changes check when checksChanges
    // (enqueueChangesCheck_). A new branch needs none: it starts where the
    // current one is, so the changes stay as they are
    void enqueueSwitch_(const QStringList& arguments, bool checksChanges)
    {
        setOperation_(Operation_::Switch);

        if (checksChanges) {
            enqueueChangesCheck_(Operation_::Switch);
        }

        Command_ command{};
        command.kind = Kind_::Step;
        command.operation = Operation_::Switch;
        command.finishes = true;
        command.folder = root_;
        command.arguments = arguments;
        command.rewritesFiles = true;
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
            dropSteps_(command.operation);

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

    // The operation underway, and busyChanged when that starts or ends one
    void setOperation_(Operation_ operation)
    {
        auto was_busy = isBusy();
        operation_ = operation;

        if (isBusy() != was_busy) {
            emit busyChanged();
        }
    }

    // An empty error is success
    void finish_(Operation_ operation, const QString& error)
    {
        setOperation_(Operation_::None);

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
