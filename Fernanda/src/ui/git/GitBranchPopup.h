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

#include <functional>
#include <utility>

#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QObject>
#include <QPoint>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>

#include "core/git/GitRepository.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The list GitHub Desktop drops from its current branch button, made for one
// showing (showBelow) and deleted when it closes: New Branch at the top, and
// Create Pull Request when there's one to open, then a field that filters the
// branches below it as it's typed in. The local branches come first, the
// current one in bold. Under a "Remote" heading come the remote branches
// that have no local branch of the same name ("origin/draft" when there's no
// "draft"). A click on a branch, or Enter on the one selected, chooses it; Up
// and Down in the field move the selection; Escape, or a click elsewhere,
// closes it. The branches are read when it shows
// (GitRepository::listBranches).
//
// It only asks: what choosing a branch, New Branch, or Create Pull Request
// means is its owner's (GitTopBar), and so is whether there's a pull request
// to offer (canOpenPullRequest), asked again each time the branches are read
class GitBranchPopup : public QWidget
{
    Q_OBJECT

public:
    GitBranchPopup(
        GitRepository* repository,
        std::function<bool()> canOpenPullRequest,
        QWidget* parentTopBar)
        : QWidget(parentTopBar, Qt::Popup)
        , repository_(repository)
        , canOpenPullRequest_(std::move(canOpenPullRequest))
    {
        setup_();
    }

    ~GitBranchPopup() override { TRACER; }

    // Show it under anchor, at least as wide, and read the branches
    void showBelow(const QWidget* anchor)
    {
        resize(
            qMax(anchor->width(), GIT_BRANCH_POPUP_MIN_WIDTH),
            GIT_BRANCH_POPUP_HEIGHT);
        move(anchor->mapToGlobal(QPoint(0, anchor->height())));
        show();

        filterEdit_->setFocus();
        repository_->listBranches();
    }

signals:
    // isRemote is true for a remote branch's name ("origin/draft")
    void branchChosen(const QString& name, bool isRemote);
    void pullRequestRequested();
    void newBranchRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape) {
            close();
            return;
        }

        QWidget::keyPressEvent(event);
    }

    // Up and Down in the field move the list's selection
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == filterEdit_ && event->type() == QEvent::KeyPress) {
            auto key = static_cast<QKeyEvent*>(event)->key();

            if (key == Qt::Key_Up || key == Qt::Key_Down) {
                QApplication::sendEvent(list_, event);
                return true;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

private:
    GitRepository* repository_;

    QLineEdit* filterEdit_ = nullptr;
    std::function<bool()> canOpenPullRequest_;

    QPushButton* pullRequestButton_ = nullptr;
    QListWidget* list_ = nullptr;

    // Every branch, as last read
    QStringList local_{};
    QStringList remote_{};

    // Where a row says it's a remote branch. A heading has no data here
    static constexpr int IS_REMOTE_ROLE_ = Qt::UserRole;

    void setup_()
    {
        setAttribute(Qt::WA_DeleteOnClose);

        auto* new_branch = new QPushButton(tr("New Branch…"), this);
        new_branch->setFlat(true);
        new_branch->setAutoDefault(false);

        pullRequestButton_ = new QPushButton(tr("Create Pull Request"), this);
        pullRequestButton_->setFlat(true);
        pullRequestButton_->setAutoDefault(false);
        pullRequestButton_->hide();

        filterEdit_ = new QLineEdit(this);
        filterEdit_->setPlaceholderText(tr("Filter"));
        filterEdit_->setClearButtonEnabled(true);
        filterEdit_->installEventFilter(this);

        list_ = new QListWidget(this);
        list_->setFrameShape(QFrame::NoFrame);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(
            GIT_BRANCH_POPUP_MARGIN,
            GIT_BRANCH_POPUP_MARGIN,
            GIT_BRANCH_POPUP_MARGIN,
            GIT_BRANCH_POPUP_MARGIN);
        layout->setSpacing(GIT_BRANCH_POPUP_MARGIN);
        layout->addWidget(new_branch);
        layout->addWidget(pullRequestButton_);
        layout->addWidget(filterEdit_);
        layout->addWidget(list_, 1);

        connect(new_branch, &QPushButton::clicked, this, [this] {
            close();
            emit newBranchRequested();
        });

        connect(pullRequestButton_, &QPushButton::clicked, this, [this] {
            close();
            emit pullRequestRequested();
        });

        connect(
            filterEdit_,
            &QLineEdit::textChanged,
            this,
            &GitBranchPopup::rebuild_);

        connect(filterEdit_, &QLineEdit::returnPressed, this, [this] {
            choose_(list_->currentItem());
        });

        connect(
            list_,
            &QListWidget::itemClicked,
            this,
            &GitBranchPopup::choose_);

        connect(
            repository_,
            &GitRepository::branchesRead,
            this,
            [this](const QStringList& local, const QStringList& remote) {
                local_ = local;
                remote_ = remote;
                pullRequestButton_->setVisible(
                    canOpenPullRequest_ && canOpenPullRequest_());
                rebuild_();
            });
    }

    // The branches whose names hold the filter's text, ignoring case, with
    // the first selected
    void rebuild_()
    {
        auto filter = filterEdit_->text().trimmed();
        const auto& current = repository_->status().branch.name;

        list_->clear();

        for (const auto& name : local_) {
            if (!name.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            auto* item = addBranch_(name, false);

            if (name == current) {
                auto font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }

        QStringList remote{};

        for (const auto& name : remote_) {
            if (name.contains(filter, Qt::CaseInsensitive) &&
                !hasLocal_(name)) {
                remote << name;
            }
        }

        if (!remote.isEmpty()) {
            auto* heading = new QListWidgetItem(tr("Remote"), list_);
            heading->setFlags(Qt::NoItemFlags);

            for (const auto& name : remote) {
                addBranch_(name, true);
            }
        }

        list_->setCurrentRow(list_->count() > 0 ? 0 : -1);
    }

    QListWidgetItem* addBranch_(const QString& name, bool isRemote)
    {
        auto* item = new QListWidgetItem(name, list_);
        item->setData(IS_REMOTE_ROLE_, isRemote);
        return item;
    }

    // Whether a remote branch ("origin/drafts/two") has a local branch of
    // the name past its remote's ("drafts/two"). A remote's name can hold a
    // slash, so it's matched against the remotes, not split off
    [[nodiscard]] bool hasLocal_(const QString& remoteBranch) const
    {
        const auto& remotes = repository_->remotes();

        for (auto it = remotes.cbegin(); it != remotes.cend(); ++it) {
            auto prefix = it.key() + u'/';

            if (remoteBranch.startsWith(prefix)) {
                return local_.contains(remoteBranch.mid(prefix.size()));
            }
        }

        return false;
    }

    // A heading has no data, and isn't chosen
    void choose_(QListWidgetItem* item)
    {
        if (!item || !item->data(IS_REMOTE_ROLE_).isValid()) {
            return;
        }

        auto name = item->text();
        auto is_remote = item->data(IS_REMOTE_ROLE_).toBool();
        close();
        emit branchChosen(name, is_remote);
    }
};

} // namespace Fernanda::Ui
