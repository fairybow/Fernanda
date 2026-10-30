/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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

#include <QAction>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QToolButton>
#include <QUrl>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/git/GitRemote.h"
#include "core/git/GitRepository.h"
#include "ui/UiConstants.h"
#include "ui/git/GitNewBranchDialog.h"
#include "ui/widgets/HoverSync.h"
#include "ui/widgets/MenuButton.h"

namespace Fernanda::Ui {

// The Git page's branch button: the branch glyph and the current branch's
// name (or that the HEAD is detached), opening a menu of what can be done with
// branches. The menu lists the local branches, the current one checked, and
// under Remote branches, those on the remote with no local branch of the same
// name ("origin/draft" when there's no "draft"). Then New branch…, which opens
// GitNewBranchDialog, and Create pull request, when there's one to open.
//
// Choosing a local branch switches to it (GitRepository::switchBranch); a
// remote one makes a local branch that tracks it and switches to that
// (switchToRemoteBranch). A switch this button asked for that fails shows
// git's reason, or the repository's own: a tracked file's changes stop a switch
// until they're committed or discarded. Disabled while the repository is busy
// with any operation.
//
// The branches are read while the button shows, and again with each new
// status while it does, so the menu is built from a recent list the moment it
// opens; an answer that comes while it's open rebuilds it in place.
//
// Create pull request shows when the branch is published to a remote on
// github.com, is still there, and isn't that remote's default branch. It
// opens GitHub's page for one in the browser (GitRemote::compareUrl), where
// the pull request is made: Fernanda sends nothing to GitHub itself
class GitBranchButton : public MenuButton
{
    Q_OBJECT

public:
    GitBranchButton(GitRepository* repository, QWidget* parentGitPage)
        : MenuButton(QString::fromLatin1(ICON_PATH_), parentGitPage)
        , repository_(repository)
    {
        setup_();
    }

    ~GitBranchButton() override { TRACER; }

protected:
    void showEvent(QShowEvent* event) override
    {
        MenuButton::showEvent(event);
        repository_->listBranches();
    }

private:
    static constexpr auto ICON_PATH_ = ":/lucide/GitBranch.svg";

    GitRepository* repository_;
    QMenu* menu_ = new QMenu(this);

    // The branches as last read
    QStringList local_{};
    QStringList remote_{};

    // Whether this button asked for a switch and awaits the answer. The New
    // Branch dialog switches too, and shows its own failures
    bool switching_ = false;

    void setup_()
    {
        setMenu(menu_);
        setPopupMode(QToolButton::InstantPopup);
        setFixedHeight(VAULT_SWITCHER_HEIGHT);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // It widens under a still pointer when the sync button beside it
        // narrows, and is disabled while the repository is busy
        HoverSync::follow(this);

        connect(
            menu_,
            &QMenu::aboutToShow,
            this,
            &GitBranchButton::rebuildMenu_);

        connect(repository_, &GitRepository::statusChanged, this, [this] {
            update_();

            if (isVisible()) {
                repository_->listBranches();
            }
        });

        connect(
            repository_,
            &GitRepository::busyChanged,
            this,
            &GitBranchButton::update_);

        connect(
            repository_,
            &GitRepository::branchesRead,
            this,
            [this](const QStringList& local, const QStringList& remote) {
                local_ = local;
                remote_ = remote;

                if (menu_->isVisible()) {
                    rebuildMenu_();
                }
            });

        connect(repository_, &GitRepository::switched, this, [this] {
            switching_ = false;
        });

        connect(
            repository_,
            &GitRepository::switchFailed,
            this,
            &GitBranchButton::onSwitchFailed_);

        update_();
    }

    // The name, and whether it can be clicked
    void update_()
    {
        const auto& branch = repository_->status().branch;
        auto name = branch.detached ? tr("Detached HEAD") : branch.name;

        setText(name);
        setToolTip(name);
        setEnabled(!repository_->isBusy());
    }

    void rebuildMenu_()
    {
        menu_->clear();

        const auto& current = repository_->status().branch.name;

        for (const auto& name : local_) {
            auto* action = menu_->addAction(name);
            action->setCheckable(true);
            action->setChecked(name == current);

            connect(action, &QAction::triggered, this, [this, name] {
                switchTo_(name, false);
            });
        }

        QStringList remote{};

        for (const auto& name : remote_) {
            if (!hasLocal_(name)) {
                remote << name;
            }
        }

        if (!remote.isEmpty()) {
            auto* submenu = menu_->addMenu(tr("Remote branches"));

            for (const auto& name : remote) {
                connect(
                    submenu->addAction(name),
                    &QAction::triggered,
                    this,
                    [this, name] { switchTo_(name, true); });
            }
        }

        if (!menu_->isEmpty()) {
            menu_->addSeparator();
        }

        connect(
            menu_->addAction(tr("New branch…")),
            &QAction::triggered,
            this,
            &GitBranchButton::showNewBranch_);

        if (auto url = pullRequestUrl_(); !url.isEmpty()) {
            connect(
                menu_->addAction(tr("Create pull request")),
                &QAction::triggered,
                this,
                [url] { QDesktopServices::openUrl(url); });
        }
    }

    // Whether a remote branch ("origin/drafts/two") has a local branch of
    // the name past its remote's ("drafts/two") (GitRepository::remoteOf)
    [[nodiscard]] bool hasLocal_(const QString& remoteBranch) const
    {
        auto remote = repository_->remoteOf(remoteBranch);

        if (remote.isEmpty()) {
            return false;
        }

        return local_.contains(remoteBranch.mid(remote.size() + 1));
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

    // Shown without exec: the window may close while it's up
    void onSwitchFailed_(const QString& error)
    {
        if (!switching_) {
            return;
        }

        switching_ = false;

        auto* box = new QMessageBox(
            QMessageBox::Warning,
            tr("Switch failed"),
            tr("The branch wasn't switched."),
            QMessageBox::Ok,
            this);
        box->setInformativeText(error);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }

    // GitHub's page for a pull request from this branch, or nothing: the
    // branch has to be published to a remote on github.com, still be there,
    // and not be that remote's default branch. The remote's name for the
    // branch is used, which may differ from the local one
    [[nodiscard]] QUrl pullRequestUrl_() const
    {
        const auto& branch = repository_->status().branch;
        auto remote = repository_->syncRemote();

        if (branch.detached || !branch.upstreamFound || remote.isEmpty() ||
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
};

} // namespace Fernanda::Ui
