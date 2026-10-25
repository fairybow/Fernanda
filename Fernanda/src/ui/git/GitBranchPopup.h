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
// showing (showBelow) and deleted when it closes: New Branch at the top, then
// a field that filters the local branches below it as it's typed in. The
// current branch is in bold. A click on a branch, or Enter on the one
// selected, chooses it; Up and Down in the field move the selection; Escape,
// or a click elsewhere, closes it. The branches are read when it shows
// (GitRepository::listBranches).
//
// It only asks: what choosing a branch or New Branch means is its owner's
// (GitTopBar)
class GitBranchPopup : public QWidget
{
    Q_OBJECT

public:
    GitBranchPopup(GitRepository* repository, QWidget* parentTopBar)
        : QWidget(parentTopBar, Qt::Popup)
        , repository_(repository)
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
    void branchChosen(const QString& name);
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
    QListWidget* list_ = nullptr;

    // Every local branch, as last read
    QStringList names_{};

    void setup_()
    {
        setAttribute(Qt::WA_DeleteOnClose);

        auto* new_branch = new QPushButton(tr("New Branch…"), this);
        new_branch->setFlat(true);
        new_branch->setAutoDefault(false);

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
        layout->addWidget(filterEdit_);
        layout->addWidget(list_, 1);

        connect(new_branch, &QPushButton::clicked, this, [this] {
            close();
            emit newBranchRequested();
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
            [this](const QStringList& names) {
                names_ = names;
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

        for (const auto& name : names_) {
            if (!name.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            auto* item = new QListWidgetItem(name, list_);

            if (name == current) {
                auto font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }

        list_->setCurrentRow(list_->count() > 0 ? 0 : -1);
    }

    void choose_(QListWidgetItem* item)
    {
        if (!item) {
            return;
        }

        auto name = item->text();
        close();
        emit branchChosen(name);
    }
};

} // namespace Fernanda::Ui
