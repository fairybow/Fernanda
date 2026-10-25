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

#include <QAbstractItemView>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "ui/UiConstants.h"
#include "ui/git/GitChangeDelegate.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// What a vault window shows in place of its sidebar and tabs while its Git
// view is open, laid out as GitHub Desktop lays out its Changes tab: a narrow
// column on the left listing the repository's changed files, and the rest on
// the right. The list covers the whole repository, which may hold more than
// the vault.
//
// It shows the repository's last status and follows statusChanged; when to
// read the status is the window's to decide. Paths are relative to the
// repository's folder, and a renamed file shows where it was and where it is
class GitView : public QWidget
{
    Q_OBJECT

public:
    GitView(GitRepository* repository, QWidget* parentVaultWindow)
        : QWidget(parentVaultWindow)
        , repository_(repository)
    {
        setup_();
    }

    ~GitView() override { TRACER; }

private:
    GitRepository* repository_;

    QLabel* countLabel_ = nullptr;
    QListWidget* list_ = nullptr;
    QLabel* placeholder_ = nullptr;

    void setup_()
    {
        auto* splitter = new QSplitter(Qt::Horizontal, this);
        splitter->addWidget(buildChangesColumn_(splitter));
        splitter->addWidget(buildDetailArea_(splitter));
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes(
            { GIT_VIEW_CHANGES_WIDTH, GIT_VIEW_CHANGES_WIDTH * 3 });

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(splitter);

        setFocusProxy(list_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitView::rebuild_);

        rebuild_();
    }

    [[nodiscard]] QWidget* buildChangesColumn_(QWidget* parentSplitter)
    {
        auto* column = new QWidget(parentSplitter);

        countLabel_ = new QLabel(column);
        countLabel_->setContentsMargins(
            GIT_VIEW_HEADER_H_MARGIN,
            GIT_VIEW_HEADER_V_MARGIN,
            GIT_VIEW_HEADER_H_MARGIN,
            GIT_VIEW_HEADER_V_MARGIN);

        list_ = new QListWidget(column);
        list_->setFrameShape(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::SingleSelection);
        list_->setTextElideMode(Qt::ElideMiddle);
        list_->setUniformItemSizes(true);
        list_->setItemDelegate(new GitChangeDelegate(list_));

        auto* layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(countLabel_);
        layout->addWidget(list_, 1);

        return column;
    }

    // Holds only a line saying there is nothing to commit, for now
    [[nodiscard]] QWidget* buildDetailArea_(QWidget* parentSplitter)
    {
        placeholder_ = new QLabel(parentSplitter);
        placeholder_->setAlignment(Qt::AlignCenter);
        placeholder_->setForegroundRole(GIT_VIEW_MUTED_TEXT_ROLE);

        return placeholder_;
    }

    // The list is made again from the status. The selected file stays
    // selected if it's still changed
    void rebuild_()
    {
        auto* current = list_->currentItem();
        auto selected =
            current ? current->data(Qt::ToolTipRole).toString() : QString{};

        list_->clear();

        const auto& entries = repository_->status().entries;

        for (const auto& entry : entries) {
            auto path = entry.path.prettyQString();

            auto* item = new QListWidgetItem(textOf_(entry), list_);
            item->setData(GitChangeDelegate::CHANGE_ROLE, int(entry.change));
            item->setToolTip(path);

            if (path == selected) {
                list_->setCurrentItem(item);
            }
        }

        countLabel_->setText(
            tr("%Ln changed file(s)", nullptr, int(entries.size())));
        placeholder_->setText(
            entries.isEmpty() ? tr("No local changes") : QString{});
    }

    [[nodiscard]] static QString textOf_(const GitStatus::Entry& entry)
    {
        if (entry.change == GitStatus::Change::Renamed) {
            return entry.originalPath.prettyQString() + u" → "_s +
                   entry.path.prettyQString();
        }

        return entry.path.prettyQString();
    }
};

} // namespace Fernanda::Ui
