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
#include <QCheckBox>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QString>
#include <QStyle>
#include <QTextDocument>
#include <QVBoxLayout>
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
// (GitChangeDelegate), and below them the commit box. The list covers the
// whole repository, which may hold more than the vault. A renamed file is
// listed at its new path.
//
// It shows the repository's last status and follows statusChanged, and asks
// for a new status each time it shows, since files may have changed while it
// was hidden. The selected file stays selected across a new status while it's
// still changed. With no changes, a line says so in the list's place.
//
// Each file has a checkbox, checked unless it's been unchecked by hand. A
// file unchecked stays unchecked across new statuses while it's listed; once
// it isn't (committed, or discarded), it's forgotten, so if it changes again
// it comes back checked. The box above the list gives the count, and checks
// every file, or unchecks every file when all are checked.
//
// The commit box takes a summary, which a commit needs, and a description,
// which it doesn't. Its button commits the checked files to the current
// branch (GitRepository::commit), as does Ctrl+Enter anywhere in the box. The
// button is disabled while there's no summary or no checked file, and while
// the repository is busy with any operation (GitRepository::busyChanged). A
// commit that fails shows git's words in a message; one that succeeds clears
// the box.
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

    // The changes, or the line in their place
    QStackedWidget* stack_ = nullptr;
    QWidget* changes_ = nullptr;
    QLabel* message_ = nullptr;

    // Shows the count of changed files too
    QCheckBox* allBox_ = nullptr;
    QListWidget* list_ = nullptr;

    QLineEdit* summaryEdit_ = nullptr;
    QPlainTextEdit* descriptionEdit_ = nullptr;
    QPushButton* commitButton_ = nullptr;

    // The status's entries the list was made from, in the list's order
    QList<GitStatus::Entry> entries_{};

    // The files unchecked by hand, by path (prettyQString), while they're
    // listed
    QSet<QString> unchecked_{};

    void setup_()
    {
        message_ = new QLabel(tr("No changes"), this);
        message_->setAlignment(Qt::AlignCenter);
        message_->setWordWrap(true);
        message_->setForegroundRole(GIT_PAGE_MUTED_TEXT_ROLE);

        stack_ = new QStackedWidget(this);
        stack_->addWidget(buildChanges_(stack_));
        stack_->addWidget(message_);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(stack_, 1);
        layout->addWidget(buildCommitBox_(this));

        setFocusProxy(list_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitPage::rebuild_);

        connect(
            repository_,
            &GitRepository::busyChanged,
            this,
            &GitPage::updateCommitBox_);

        connect(
            repository_,
            &GitRepository::committed,
            this,
            &GitPage::onCommitted_);

        connect(
            repository_,
            &GitRepository::commitFailed,
            this,
            &GitPage::onCommitFailed_);

        rebuild_();
    }

    // The check-all box over the list
    [[nodiscard]] QWidget* buildChanges_(QWidget* parentStack)
    {
        changes_ = new QWidget(parentStack);

        list_ = new QListWidget(changes_);
        list_->setFrameShape(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::SingleSelection);
        list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list_->setUniformItemSizes(true);
        list_->setItemDelegate(new GitChangeDelegate(list_));
        list_->setContextMenuPolicy(Qt::CustomContextMenu);

        // A checkbox paints across its whole rect and takes no margins of its
        // own, so they're the layout's around it. Its left edge is where the
        // style puts a row's checkbox: the list's text margin from the row's
        // edge (PM_FocusFrameHMargin, plus one)
        auto* header = new QWidget(changes_);
        auto* header_layout = new QHBoxLayout(header);
        auto row_margin =
            style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, list_) +
            1;
        header_layout->setContentsMargins(
            row_margin,
            GIT_PAGE_HEADER_V_MARGIN,
            row_margin,
            GIT_PAGE_HEADER_V_MARGIN);

        // Partly checked shows that some files are and some aren't. A click
        // is settled by onAllBoxClicked_, not left to the box's own cycle,
        // which would go to partly checked after unchecked
        allBox_ = new QCheckBox(header);
        allBox_->setTristate(true);
        header_layout->addWidget(allBox_);

        auto* layout = new QVBoxLayout(changes_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(header);
        layout->addWidget(list_, 1);

        connect(allBox_, &QCheckBox::clicked, this, &GitPage::onAllBoxClicked_);

        connect(
            list_,
            &QListWidget::itemChanged,
            this,
            &GitPage::onItemChanged_);

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

        return changes_;
    }

    [[nodiscard]] QWidget* buildCommitBox_(QWidget* parentPage)
    {
        auto* box = new QWidget(parentPage);

        summaryEdit_ = new QLineEdit(box);
        summaryEdit_->setPlaceholderText(tr("Summary (required)"));

        descriptionEdit_ = new QPlainTextEdit(box);
        descriptionEdit_->setPlaceholderText(tr("Description"));
        descriptionEdit_->setTabChangesFocus(true);
        descriptionEdit_->setFixedHeight(
            QFontMetrics(descriptionEdit_->font()).lineSpacing() *
                GIT_PAGE_DESCRIPTION_LINES +
            2 * descriptionEdit_->frameWidth() +
            2 * int(descriptionEdit_->document()->documentMargin()));

        // The default button's look marks it as the box's one action. Its
        // width doesn't widen the sidebar: a long branch name is cut off,
        // and the tooltip has it whole
        commitButton_ = new QPushButton(box);
        commitButton_->setDefault(true);
        commitButton_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        connect(
            summaryEdit_,
            &QLineEdit::textChanged,
            this,
            &GitPage::updateCommitBox_);

        connect(commitButton_, &QPushButton::clicked, this, &GitPage::commit_);

        // Ctrl+Enter commits from anywhere in the box, as in GitHub Desktop
        for (const auto& keys : { QKeySequence(u"Ctrl+Return"_s),
                                  QKeySequence(u"Ctrl+Enter"_s) }) {
            auto* shortcut = new QShortcut(keys, box);
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated, this, &GitPage::commit_);
        }

        auto* layout = new QVBoxLayout(box);
        layout->setContentsMargins(
            GIT_PAGE_COMMIT_MARGIN,
            GIT_PAGE_COMMIT_MARGIN,
            GIT_PAGE_COMMIT_MARGIN,
            GIT_PAGE_COMMIT_MARGIN);
        layout->setSpacing(GIT_PAGE_COMMIT_SPACING);
        layout->addWidget(summaryEdit_);
        layout->addWidget(descriptionEdit_);
        layout->addWidget(commitButton_);

        return box;
    }

    // The list made again from the status, with its signals held: the
    // selected file kept selected if it's still listed, and each file
    // checked unless it was unchecked by hand. A file no longer listed is
    // forgotten from the unchecked
    void rebuild_()
    {
        Coco::Path selected{};

        if (auto row = list_->currentRow(); row >= 0 && row < entries_.size()) {
            selected = entries_[row].path;
        }

        entries_ = repository_->status().entries;

        QSet<QString> listed{};
        auto selected_row = -1;

        {
            QSignalBlocker blocker(list_);
            list_->clear();

            for (const auto& entry : entries_) {
                auto path = entry.path.prettyQString();
                listed << path;

                auto* item =
                    new QListWidgetItem(entry.path.nameQString(), list_);
                item->setData(
                    GitChangeDelegate::CHANGE_ROLE,
                    int(entry.change));
                item->setData(
                    GitChangeDelegate::FOLDER_ROLE,
                    entry.path.parent().prettyQString());
                item->setToolTip(toolTipOf_(entry));
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(
                    unchecked_.contains(path) ? Qt::Unchecked : Qt::Checked);

                if (entry.path == selected) {
                    selected_row = list_->count() - 1;
                }
            }

            list_->setCurrentRow(selected_row);
        }

        unchecked_.intersect(listed);

        allBox_->setText(
            tr("%Ln changed file(s)", nullptr, int(entries_.size())));

        if (entries_.isEmpty()) {
            stack_->setCurrentWidget(message_);
        } else {
            stack_->setCurrentWidget(changes_);
        }

        updateCommitBox_();
    }

    // A file checked or unchecked by hand: its box, or Space
    void onItemChanged_(QListWidgetItem* item)
    {
        auto row = list_->row(item);

        if (row < 0 || row >= entries_.size()) {
            return;
        }

        auto path = entries_[row].path.prettyQString();

        if (item->checkState() == Qt::Checked) {
            unchecked_.remove(path);
        } else {
            unchecked_.insert(path);
        }

        updateCommitBox_();
    }

    // All checked unchecks them all; anything else checks them all
    void onAllBoxClicked_()
    {
        auto check = checkedEntries_().size() != entries_.size();

        {
            QSignalBlocker blocker(list_);

            for (auto row = 0; row < list_->count(); ++row) {
                list_->item(row)->setCheckState(
                    check ? Qt::Checked : Qt::Unchecked);
            }
        }

        for (const auto& entry : entries_) {
            if (check) {
                unchecked_.remove(entry.path.prettyQString());
            } else {
                unchecked_.insert(entry.path.prettyQString());
            }
        }

        updateCommitBox_();
    }

    // The checked files' entries, in the list's order
    [[nodiscard]] QList<GitStatus::Entry> checkedEntries_() const
    {
        QList<GitStatus::Entry> entries{};

        for (auto row = 0; row < list_->count() && row < entries_.size();
             ++row) {
            if (list_->item(row)->checkState() == Qt::Checked) {
                entries << entries_[row];
            }
        }

        return entries;
    }

    // The check-all box's state, and the button: its branch, and whether
    // there's a summary and a checked file to commit, with nothing underway
    void updateCommitBox_()
    {
        auto checked = checkedEntries_().size();

        allBox_->setCheckState(
            checked == 0                 ? Qt::Unchecked
            : checked == entries_.size() ? Qt::Checked
                                         : Qt::PartiallyChecked);

        const auto& branch = repository_->status().branch;
        auto text = branch.name.isEmpty() ? tr("Commit")
                                          : tr("Commit to %1").arg(branch.name);
        commitButton_->setText(text);
        commitButton_->setToolTip(text);

        commitButton_->setEnabled(
            !repository_->isBusy() && checked > 0 &&
            !summaryEdit_->text().trimmed().isEmpty());
    }

    void commit_()
    {
        if (!commitButton_->isEnabled()) {
            return;
        }

        repository_->commit(
            checkedEntries_(),
            summaryEdit_->text().trimmed(),
            descriptionEdit_->toPlainText().trimmed());
    }

    // The status is read again after either answer, which rebuilds the list
    void onCommitted_()
    {
        summaryEdit_->clear();
        descriptionEdit_->clear();
    }

    // Shown without exec: the window may close while it's up
    void onCommitFailed_(const QString& error)
    {
        auto* box = new QMessageBox(
            QMessageBox::Warning,
            tr("Commit failed"),
            tr("Nothing was committed."),
            QMessageBox::Ok,
            this);
        box->setInformativeText(error);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
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
