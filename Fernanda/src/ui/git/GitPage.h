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

#include <QAbstractItemView>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPoint>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStackedLayout>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/PathKey.h"
#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "ui/OpenMode.h"
#include "ui/UiConstants.h"
#include "ui/git/GitChangeDelegate.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The sidebar's Git page: the repository's changed files, one row each
// (GitChangeDelegate). The list covers the whole repository, which may hold
// more than the vault. A renamed file is listed at its new path.
//
// It shows the repository's last status and follows statusChanged, and asks
// for a new status each time it shows, since files may have changed while it
// was hidden. The selected file stays selected across a new status while it's
// still changed. With no changes, a line says so in the list's place.
//
// A file's diff opens as the file tree opens a file: a double-click, or Enter,
// opens it over the active tab unless that one is pinned, and the right-click
// menu opens it in a new tab, to the right, or in a new window. The same menu
// opens the file itself, when the window says it can (setOpenFilter). The page
// only asks (diffRequested, fileRequested); the window opens
class GitPage : public QWidget
{
    Q_OBJECT

public:
    explicit GitPage(GitRepository* repository, QWidget* parentSidebar)
        : QWidget(parentSidebar)
        , repository_(repository)
    {
        setup_();
    }

    ~GitPage() override { TRACER; }

    // Which files the right-click menu may offer to open. None until set
    void setOpenFilter(std::function<bool(const PathKey&)> canOpen)
    {
        canOpen_ = std::move(canOpen);
    }

signals:
    // Open the diff of the file entry names, placed as mode says
    void diffRequested(const Fernanda::GitStatus::Entry& entry, OpenMode mode);

    // Open the file at key itself. Asked only for a file the open filter
    // allows
    void fileRequested(const PathKey& key);

protected:
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        repository_->refresh();
    }

private:
    GitRepository* repository_;
    std::function<bool(const PathKey&)> canOpen_{};

    QStackedLayout* stack_ = nullptr;
    QListWidget* list_ = nullptr;
    QLabel* message_ = nullptr;

    // The status's entries the list was made from, in the list's order
    QList<GitStatus::Entry> entries_{};

    void setup_()
    {
        list_ = new QListWidget(this);
        list_->setFrameShape(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::SingleSelection);
        list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list_->setUniformItemSizes(true);
        list_->setItemDelegate(new GitChangeDelegate(list_));
        list_->setContextMenuPolicy(Qt::CustomContextMenu);

        message_ = new QLabel(tr("No changes"), this);
        message_->setAlignment(Qt::AlignCenter);
        message_->setWordWrap(true);
        message_->setForegroundRole(GIT_PAGE_MUTED_TEXT_ROLE);

        stack_ = new QStackedLayout(this);
        stack_->addWidget(list_);
        stack_->addWidget(message_);

        setFocusProxy(list_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitPage::rebuild_);

        // A double-click, or Enter
        connect(
            list_,
            &QListWidget::itemActivated,
            this,
            [this](QListWidgetItem* item) {
                requestDiff_(list_->row(item), OpenMode::ReplaceActive);
            });

        connect(
            list_,
            &QWidget::customContextMenuRequested,
            this,
            &GitPage::showContextMenu_);

        rebuild_();
    }

    // The list made again from the status, keeping the selected file selected
    // if it's still listed
    void rebuild_()
    {
        Coco::Path selected{};

        if (auto row = list_->currentRow(); row >= 0 && row < entries_.size()) {
            selected = entries_[row].path;
        }

        entries_ = repository_->status().entries;

        QSignalBlocker blocker(list_);
        list_->clear();

        auto selected_row = -1;

        for (const auto& entry : entries_) {
            auto* item = new QListWidgetItem(entry.path.nameQString(), list_);
            item->setData(GitChangeDelegate::CHANGE_ROLE, int(entry.change));
            item->setData(
                GitChangeDelegate::FOLDER_ROLE,
                entry.path.parent().prettyQString());
            item->setToolTip(toolTipOf_(entry));

            if (entry.path == selected) {
                selected_row = list_->count() - 1;
            }
        }

        list_->setCurrentRow(selected_row);
        if (entries_.isEmpty()) {
            stack_->setCurrentWidget(message_);
        } else {
            stack_->setCurrentWidget(list_);
        }
    }

    void requestDiff_(int row, OpenMode mode)
    {
        if (row >= 0 && row < entries_.size()) {
            emit diffRequested(entries_[row], mode);
        }
    }

    // The file at row as a key, if the window says it can be opened
    [[nodiscard]] PathKey openableKey_(int row) const
    {
        if (!canOpen_ || row < 0 || row >= entries_.size()) {
            return {};
        }

        auto key = PathKey::of(repository_->root() / entries_[row].path);
        return canOpen_(key) ? key : PathKey{};
    }

    // The diff's three placements, as the file tree's menu offers a file's,
    // then the file itself
    void showContextMenu_(const QPoint& position)
    {
        auto* item = list_->itemAt(position);

        if (!item) {
            return;
        }

        auto row = list_->row(item);
        QMenu menu(this);

        menu.addAction(tr("Open in new tab"), this, [this, row] {
            requestDiff_(row, OpenMode::NewTab);
        });

        menu.addAction(tr("Open to the right"), this, [this, row] {
            requestDiff_(row, OpenMode::SplitRight);
        });

        menu.addAction(tr("Open in new window"), this, [this, row] {
            requestDiff_(row, OpenMode::NewWindow);
        });

        if (auto key = openableKey_(row); !key.isEmpty()) {
            menu.addSeparator();
            menu.addAction(tr("Open file"), this, [this, key] {
                emit fileRequested(key);
            });
        }

        menu.exec(list_->viewport()->mapToGlobal(position));
    }

    // The path from the repository's folder, and for a renamed file, where
    // it was
    [[nodiscard]] static QString toolTipOf_(const GitStatus::Entry& entry)
    {
        if (entry.change == GitStatus::Change::Renamed) {
            return tr("%1 (renamed from %2)")
                .arg(
                    entry.path.prettyQString(),
                    entry.originalPath.prettyQString());
        }

        return entry.path.prettyQString();
    }
};

} // namespace Fernanda::Ui
