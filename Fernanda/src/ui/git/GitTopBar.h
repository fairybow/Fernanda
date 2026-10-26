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

#include <QAbstractButton>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QUrl>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/git/GitRemote.h"
#include "core/git/GitRepository.h"
#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/git/GitBarItem.h"
#include "ui/git/GitBranchPopup.h"
#include "ui/git/GitNewBranchDialog.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The Git view's top bar, after GitHub Desktop's: the repository, which is
// the window's vault's and fixed, so only shown; the current branch, which
// drops a list of branches to switch to, or to make a new one from
// (GitBranchPopup); and one button that syncs with the branch's remote. A
// line runs under the bar and after each section.
//
// The names follow the repository's status. A detached HEAD is named so;
// before the first commit, the branch is the one git will make, which git
// already names.
//
// Choosing a branch asks the repository to switch (GitRepository::
// switchBranch, or switchToRemoteBranch for a remote one); New Branch opens
// GitNewBranchDialog, which asks it to make one. A switch this bar asked for
// that fails shows git's reason, or the repository's own: a tracked file's
// changes stop a switch until they're committed or discarded.
//
// The sync button does what the branch needs, as GitHub Desktop's does, in
// this order: publish a branch that has no upstream; pull when the remote
// has commits this branch doesn't; push when this branch has commits the
// remote doesn't; otherwise fetch. Its caption says when the repository last
// fetched, from git, so a fetch in a terminal counts too. It does nothing
// until clicked: Fernanda never fetches on its own. While it runs it says
// what it's doing, and a failure shows git's words. With no remote, it says
// so and does nothing.
//
// The branch list offers Create Pull Request when the branch is published to
// a remote on github.com and isn't that remote's default branch. It opens
// GitHub's page for one in the browser (GitRemote::compareUrl), where the
// pull request is made: Fernanda sends nothing to GitHub itself
class GitTopBar : public QWidget
{
    Q_OBJECT

public:
    GitTopBar(GitRepository* repository, QWidget* parentGitView)
        : QWidget(parentGitView)
        , repository_(repository)
    {
        setup_();
    }

    ~GitTopBar() override { TRACER; }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setPen(QPen(Style::divider(palette()), 1.0));

        auto bottom = height() - 1;
        painter.drawLine(0, bottom, width(), bottom);

        // In the one-pixel gap the layout leaves after each section
        for (const auto* item : { repositoryItem_, branchItem_, syncItem_ }) {
            auto x = item->geometry().right() + 1;
            painter.drawLine(x, 0, x, bottom);
        }
    }

private:
    using SyncAction_ = GitRepository::SyncAction;

    GitRepository* repository_;

    GitBarItem* repositoryItem_ = nullptr;
    GitBarItem* branchItem_ = nullptr;
    GitBarItem* syncItem_ = nullptr;

    // Whether this bar asked for a switch, or a sync, and awaits the answer
    bool switching_ = false;
    bool syncing_ = false;

    // What the sync button does when clicked, as last shown
    SyncAction_ syncAction_ = SyncAction_::Fetch;

    void setup_()
    {
        repositoryItem_ = new GitBarItem(
            u":/lucide/FolderGit2.svg"_s,
            tr("Current repository"),
            GitBarItem::Role::Label,
            this);

        branchItem_ = new GitBarItem(
            u":/lucide/GitBranch.svg"_s,
            tr("Current branch"),
            GitBarItem::Role::Menu,
            this);

        syncItem_ = new GitBarItem(
            u":/lucide/RefreshCw.svg"_s,
            {},
            GitBarItem::Role::Button,
            this);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 1);
        layout->setSpacing(1);
        layout->addWidget(repositoryItem_);
        layout->addWidget(branchItem_);
        layout->addWidget(syncItem_);
        layout->addStretch(1);

        connect(
            branchItem_,
            &QAbstractButton::clicked,
            this,
            &GitTopBar::showBranches_);

        connect(syncItem_, &QAbstractButton::clicked, this, &GitTopBar::sync_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitTopBar::updateAll_);

        connect(
            repository_,
            &GitRepository::foundChanged,
            this,
            &GitTopBar::updateAll_);

        connect(
            repository_,
            &GitRepository::remotesChanged,
            this,
            &GitTopBar::updateAll_);

        connect(repository_, &GitRepository::switched, this, [this] {
            switching_ = false;
        });

        connect(
            repository_,
            &GitRepository::switchFailed,
            this,
            &GitTopBar::onSwitchFailed_);

        connect(repository_, &GitRepository::synced, this, [this] {
            syncing_ = false;
            updateSync_();
        });

        connect(
            repository_,
            &GitRepository::syncFailed,
            this,
            &GitTopBar::onSyncFailed_);

        updateAll_();
    }

    void updateAll_()
    {
        repositoryItem_->setName(repository_->root().nameQString());

        const auto& branch = repository_->status().branch;
        branchItem_->setName(
            branch.detached ? tr("Detached HEAD") : branch.name);

        updateSync_();
    }

    // The sync button's glyph, caption, and name, and what a click does
    void updateSync_()
    {
        auto remote = repository_->syncRemote();
        const auto& branch = repository_->status().branch;

        if (auto running = repository_->syncing()) {
            syncItem_->setEnabled(false);
            syncItem_->setGlyph(u":/lucide/LoaderCircle.svg"_s);
            syncItem_->setCaption(remote);
            syncItem_->setName(runningName_(*running));
            return;
        }

        if (remote.isEmpty()) {
            syncItem_->setEnabled(false);
            syncItem_->setGlyph(u":/lucide/RefreshCw.svg"_s);
            syncItem_->setCaption(tr("No remote"));
            syncItem_->setName(tr("Fetch"));
            return;
        }

        syncItem_->setEnabled(true);
        syncItem_->setCaption(lastFetchedText_());

        if (branch.upstream.isEmpty() && !branch.detached && !branch.unborn) {
            syncAction_ = SyncAction_::Publish;
            syncItem_->setGlyph(u":/lucide/CloudUpload.svg"_s);
            syncItem_->setCaption(tr("Publish this branch to %1").arg(remote));
            syncItem_->setName(tr("Publish branch"));
        } else if (branch.behind > 0) {
            syncAction_ = SyncAction_::Pull;
            syncItem_->setGlyph(u":/lucide/ArrowDown.svg"_s);
            syncItem_->setName(
                tr("Pull %1 ↓%2").arg(remote, QString::number(branch.behind)));
        } else if (branch.ahead > 0) {
            syncAction_ = SyncAction_::Push;
            syncItem_->setGlyph(u":/lucide/ArrowUp.svg"_s);
            syncItem_->setName(
                tr("Push %1 ↑%2").arg(remote, QString::number(branch.ahead)));
        } else {
            syncAction_ = SyncAction_::Fetch;
            syncItem_->setGlyph(u":/lucide/RefreshCw.svg"_s);
            syncItem_->setName(tr("Fetch %1").arg(remote));
        }
    }

    [[nodiscard]] QString runningName_(SyncAction_ action) const
    {
        switch (action) {
        case SyncAction_::Fetch:
            return tr("Fetching…");
        case SyncAction_::Pull:
            return tr("Pulling…");
        case SyncAction_::Push:
            return tr("Pushing…");
        case SyncAction_::Publish:
            return tr("Publishing…");
        }

        return {};
    }

    // How long ago the repository last fetched, roughly, as GitHub Desktop
    // says it
    [[nodiscard]] QString lastFetchedText_() const
    {
        auto fetched = repository_->lastFetchedEpochMs();

        if (fetched <= 0) {
            return tr("Never fetched");
        }

        auto minutes = int((Coco::Time::epoch() - fetched) / (60 * 1000));

        if (minutes < 1) {
            return tr("Last fetched just now");
        }

        if (minutes < 60) {
            return tr("Last fetched %Ln minute(s) ago", nullptr, minutes);
        }

        if (minutes < 24 * 60) {
            return tr("Last fetched %Ln hour(s) ago", nullptr, minutes / 60);
        }

        return tr("Last fetched %Ln day(s) ago", nullptr, minutes / (24 * 60));
    }

    void sync_()
    {
        if (repository_->isBusy()) {
            return;
        }

        syncing_ = true;
        repository_->sync(syncAction_);
        updateSync_();
    }

    // Shown without exec: the window may close while it's up
    void onSyncFailed_(const QString& error)
    {
        updateSync_();

        if (!syncing_) {
            return;
        }

        syncing_ = false;
        showFailure_(tr("Sync failed"), syncFailureText_(), error);
    }

    [[nodiscard]] QString syncFailureText_() const
    {
        switch (syncAction_) {
        case SyncAction_::Fetch:
            return tr("Nothing was fetched.");
        case SyncAction_::Pull:
            return tr(
                "Nothing was pulled. If both this branch and its remote "
                "have new commits, they need merging outside Fernanda.");
        case SyncAction_::Push:
            return tr("Nothing was pushed.");
        case SyncAction_::Publish:
            return tr("The branch wasn't published.");
        }

        return {};
    }

    // Not while a commit, discard, switch, or sync is underway: what's listed
    // and what's current could change under it
    void showBranches_()
    {
        if (repository_->isBusy()) {
            return;
        }

        auto* popup = new GitBranchPopup(
            repository_,
            [this] { return !pullRequestUrl_().isEmpty(); },
            this);

        connect(
            popup,
            &GitBranchPopup::branchChosen,
            this,
            &GitTopBar::switchTo_);

        connect(
            popup,
            &GitBranchPopup::newBranchRequested,
            this,
            &GitTopBar::showNewBranch_);

        connect(popup, &GitBranchPopup::pullRequestRequested, this, [this] {
            if (auto url = pullRequestUrl_(); !url.isEmpty()) {
                QDesktopServices::openUrl(url);
            }
        });

        popup->showBelow(branchItem_);
    }

    // GitHub's page for a pull request from this branch, or nothing: the
    // branch has to be published to a remote on github.com, and not be that
    // remote's default branch. The remote's name for the branch is used,
    // which may differ from the local one
    [[nodiscard]] QUrl pullRequestUrl_() const
    {
        const auto& branch = repository_->status().branch;
        auto remote = repository_->syncRemote();

        if (branch.detached || remote.isEmpty() ||
            !branch.upstream.startsWith(remote + u'/')) {
            return {};
        }

        auto repo =
            GitRemote::gitHubRepoOf(repository_->remotes().value(remote));

        if (!repo) {
            return {};
        }

        auto upstream_branch = branch.upstream.mid(remote.size() + 1);

        if (upstream_branch == repository_->defaultBranchOf(remote)) {
            return {};
        }

        return GitRemote::compareUrl(*repo, upstream_branch);
    }

    void switchTo_(const QString& name, bool isRemote)
    {
        if (repository_->isBusy() ||
            (!isRemote && name == repository_->status().branch.name)) {
            return;
        }

        switching_ = true;

        if (isRemote) {
            repository_->switchToRemoteBranch(name);
        } else {
            repository_->switchBranch(name);
        }
    }

    void showNewBranch_()
    {
        auto* dialog = new GitNewBranchDialog(repository_, window());
        dialog->open();
    }

    void onSwitchFailed_(const QString& error)
    {
        if (!switching_) {
            return;
        }

        switching_ = false;
        showFailure_(
            tr("Switch failed"),
            tr("The branch wasn't switched."),
            error);
    }

    // Shown without exec: the window may close while it's up
    void showFailure_(
        const QString& title,
        const QString& text,
        const QString& error)
    {
        auto* box = new QMessageBox(
            QMessageBox::Warning,
            title,
            text,
            QMessageBox::Ok,
            this);
        box->setInformativeText(error);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }
};

} // namespace Fernanda::Ui
