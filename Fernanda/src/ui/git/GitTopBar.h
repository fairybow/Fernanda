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
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/git/GitRepository.h"
#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/git/GitBarItem.h"
#include "ui/git/GitBranchPopup.h"
#include "ui/git/GitNewBranchDialog.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The Git view's top bar, after GitHub Desktop's: the repository, which is
// the window's vault's and fixed, so only shown, then the current branch,
// which drops a list of branches to switch to, or to make a new one from
// (GitBranchPopup). A line runs under the bar and after each section.
//
// The names follow the repository's status. A detached HEAD is named so;
// before the first commit, the branch is the one git will make, which git
// already names.
//
// Choosing a branch asks the repository to switch (GitRepository::
// switchBranch); New Branch opens GitNewBranchDialog, which asks it to make
// one. A switch this bar asked for that fails shows git's reason, or the
// repository's own: a tracked file's changes stop a switch until they're
// committed or discarded
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
        for (const auto* item : { repositoryItem_, branchItem_ }) {
            auto x = item->geometry().right() + 1;
            painter.drawLine(x, 0, x, bottom);
        }
    }

private:
    GitRepository* repository_;

    GitBarItem* repositoryItem_ = nullptr;
    GitBarItem* branchItem_ = nullptr;

    // Whether this bar asked for a switch and awaits the answer
    bool switching_ = false;

    void setup_()
    {
        repositoryItem_ = new GitBarItem(
            u":/lucide/FolderGit2.svg"_s,
            tr("Current repository"),
            false,
            this);

        branchItem_ = new GitBarItem(
            u":/lucide/GitBranch.svg"_s,
            tr("Current branch"),
            true,
            this);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 1);
        layout->setSpacing(1);
        layout->addWidget(repositoryItem_);
        layout->addWidget(branchItem_);
        layout->addStretch(1);

        connect(
            branchItem_,
            &QAbstractButton::clicked,
            this,
            &GitTopBar::showBranches_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitTopBar::updateNames_);

        connect(
            repository_,
            &GitRepository::foundChanged,
            this,
            &GitTopBar::updateNames_);

        connect(repository_, &GitRepository::switched, this, [this] {
            switching_ = false;
        });

        connect(
            repository_,
            &GitRepository::switchFailed,
            this,
            &GitTopBar::onSwitchFailed_);

        updateNames_();
    }

    void updateNames_()
    {
        repositoryItem_->setName(repository_->root().nameQString());

        const auto& branch = repository_->status().branch;
        branchItem_->setName(
            branch.detached ? tr("Detached HEAD") : branch.name);
    }

    // Not while a commit, discard, or switch is underway: what's listed and
    // what's current could change under it
    void showBranches_()
    {
        if (repository_->isBusy()) {
            return;
        }

        auto* popup = new GitBranchPopup(repository_, this);

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

        popup->showBelow(branchItem_);
    }

    void switchTo_(const QString& name)
    {
        if (name == repository_->status().branch.name ||
            repository_->isBusy()) {
            return;
        }

        switching_ = true;
        repository_->switchBranch(name);
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
};

} // namespace Fernanda::Ui
