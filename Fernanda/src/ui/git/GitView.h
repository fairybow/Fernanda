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
#include <QFont>
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
#include <QSplitter>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/PathKey.h"
#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "core/git/GitDiff.h"
#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "ui/UiConstants.h"
#include "ui/git/GitChangeDelegate.h"
#include "ui/git/GitDiffView.h"
#include "ui/git/GitTopBar.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// What a vault window shows in place of its sidebar and tabs while its Git
// view is open, laid out as GitHub Desktop lays out its Changes tab: a bar
// across the top naming the repository and its branch (GitTopBar), then a
// narrow column on the left listing the repository's changed files, and the
// rest on the right. The list covers the whole repository, which may hold more
// than the vault.
//
// It shows the repository's last status and follows statusChanged; when to
// read the status is the window's to decide. Paths are relative to the
// repository's folder, and a renamed file shows where it was and where it is.
//
// The selected file's diff is on the right (GitDiffView), asked for each time
// a file is selected, again when a new status leaves it selected, since the
// file may have changed, and again when the view shows. A hidden view asks for
// none: the window keeps it, hidden, after it's first opened. The same file's
// diff read again keeps where it was scrolled to. A file stays selected across
// a new status while it's still changed; with none selected, the first is, as
// in GitHub Desktop.
// In the diff's place, a line says when there's nothing to show: no changes,
// a binary file, a file whose change has no lines (an empty file, or only its
// mode changed), or git's own message when it failed.
//
// The diff is in the vault's text font, so prose reads as it does in the
// editor, following the vault's settings.
//
// Each file has a checkbox, checked unless unchecked by hand, and a box above
// the list checks or unchecks them all. A file unchecked stays unchecked
// across new statuses until checked again, for as long as the view lasts.
// Below the list, the commit box: a summary, which a commit needs, a
// description, which it doesn't, and a button that commits the checked files
// to the current branch (GitRepository::commit). Ctrl+Enter in the box does
// the same. The button is disabled while the repository is busy with any
// operation (GitRepository::busyChanged). What the commit's git said, when it
// fails, is shown in a message.
//
// A file can be opened from its right-click menu, or by double-clicking it.
// The view only asks (openRequested); which files can be opened is the
// window's to say (setOpenFilter), and Open is offered only for those.
//
// The same menu discards a file's changes (GitRepository::discard), after
// asking, with Cancel the default. The question says where the changes go:
// to the trash, the file going back to its last commit, or, for a file the
// last commit doesn't have, the file itself to the trash. Not offered for a
// conflicted file, or while the repository is busy
class GitView : public QWidget
{
    Q_OBJECT

public:
    // vault is the window's own, whose settings give the diff its font
    GitView(
        GitRepository* repository,
        Vault* vault,
        QWidget* parentVaultWindowStack)
        : QWidget(parentVaultWindowStack)
        , repository_(repository)
        , vault_(vault)
    {
        setup_();
    }

    ~GitView() override { TRACER; }

    // Which files the right-click menu and a double-click may ask to open.
    // None until set
    void setOpenFilter(std::function<bool(const PathKey&)> canOpen)
    {
        canOpen_ = std::move(canOpen);
    }

signals:
    // Open the file at key. Asked from the right-click menu or a
    // double-click, only for a file the open filter allows
    void openRequested(const PathKey& key);

protected:
    // Diffs aren't read while hidden, so the selected file's is read now
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        readSelectedDiff_();
    }

private:
    GitRepository* repository_;
    Vault* vault_;
    std::function<bool(const PathKey&)> canOpen_{};

    // Shows the count of changed files too
    QCheckBox* allBox_ = nullptr;
    QListWidget* list_ = nullptr;

    QLineEdit* summaryEdit_ = nullptr;
    QPlainTextEdit* descriptionEdit_ = nullptr;
    QPushButton* commitButton_ = nullptr;

    QStackedWidget* detail_ = nullptr;
    QLabel* message_ = nullptr;
    GitDiffView* diffView_ = nullptr;

    // The status's entries the list was made from, in the list's order
    QList<GitStatus::Entry> entries_{};

    // The selected file's path, whose diff is wanted. Empty with none selected
    Coco::Path selectedPath_{};

    // The file whose diff the diff view shows. Empty while a message shows in
    // its place
    Coco::Path shownPath_{};

    // The files unchecked by hand, by path (prettyQString). They stay
    // unchecked while listed again, and while not
    QSet<QString> excluded_{};

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
        layout->setSpacing(0);
        layout->addWidget(new GitTopBar(repository_, this));
        layout->addWidget(splitter, 1);

        setFocusProxy(list_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitView::rebuild_);

        connect(
            repository_,
            &GitRepository::diffRead,
            this,
            &GitView::onDiffRead_);

        connect(
            repository_,
            &GitRepository::diffFailed,
            this,
            &GitView::onDiffFailed_);

        connect(
            list_,
            &QListWidget::currentRowChanged,
            this,
            &GitView::onSelectionChanged_);

        connect(
            list_,
            &QListWidget::itemChanged,
            this,
            &GitView::onItemChanged_);

        connect(
            list_,
            &QListWidget::itemDoubleClicked,
            this,
            [this](QListWidgetItem* item) { requestOpen_(list_->row(item)); });

        connect(
            list_,
            &QWidget::customContextMenuRequested,
            this,
            &GitView::showContextMenu_);

        connect(
            repository_,
            &GitRepository::committed,
            this,
            &GitView::onCommitted_);

        connect(
            repository_,
            &GitRepository::commitFailed,
            this,
            &GitView::onCommitFailed_);

        connect(
            repository_,
            &GitRepository::busyChanged,
            this,
            &GitView::updateCommitBox_);

        connect(
            repository_,
            &GitRepository::discardFailed,
            this,
            &GitView::onDiscardFailed_);

        applyFont_();
        connect(vault_, &Vault::configChanged, this, &GitView::applyFont_);

        rebuild_();
    }

    [[nodiscard]] QWidget* buildChangesColumn_(QWidget* parentSplitter)
    {
        auto* column = new QWidget(parentSplitter);

        // A checkbox paints across its whole rect and takes no margins of
        // its own, so they're the layout's around it
        auto* header = new QWidget(column);
        auto* header_layout = new QHBoxLayout(header);
        header_layout->setContentsMargins(
            GIT_VIEW_HEADER_H_MARGIN,
            GIT_VIEW_HEADER_V_MARGIN,
            GIT_VIEW_HEADER_H_MARGIN,
            GIT_VIEW_HEADER_V_MARGIN);

        // Partly checked shows that some files are and some aren't. A click
        // is settled by onAllBoxClicked_, not left to the box's own cycle,
        // which would go to partly checked after unchecked
        allBox_ = new QCheckBox(header);
        allBox_->setTristate(true);
        header_layout->addWidget(allBox_);

        connect(allBox_, &QCheckBox::clicked, this, &GitView::onAllBoxClicked_);

        list_ = new QListWidget(column);
        list_->setFrameShape(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::SingleSelection);
        list_->setTextElideMode(Qt::ElideMiddle);
        list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list_->setUniformItemSizes(true);
        list_->setItemDelegate(new GitChangeDelegate(list_));
        list_->setContextMenuPolicy(Qt::CustomContextMenu);

        auto* layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(header);
        layout->addWidget(list_, 1);
        layout->addWidget(buildCommitBox_(column));

        return column;
    }

    [[nodiscard]] QWidget* buildCommitBox_(QWidget* parentColumn)
    {
        auto* box = new QWidget(parentColumn);

        summaryEdit_ = new QLineEdit(box);
        summaryEdit_->setPlaceholderText(tr("Summary (required)"));

        descriptionEdit_ = new QPlainTextEdit(box);
        descriptionEdit_->setPlaceholderText(tr("Description"));
        descriptionEdit_->setTabChangesFocus(true);
        descriptionEdit_->setFixedHeight(
            QFontMetrics(descriptionEdit_->font()).lineSpacing() *
                GIT_VIEW_DESCRIPTION_LINES +
            2 * descriptionEdit_->frameWidth() +
            2 * int(descriptionEdit_->document()->documentMargin()));

        commitButton_ = new QPushButton(box);

        connect(
            summaryEdit_,
            &QLineEdit::textChanged,
            this,
            &GitView::updateCommitBox_);

        connect(commitButton_, &QPushButton::clicked, this, &GitView::commit_);

        // Ctrl+Enter commits from anywhere in the box, as in GitHub Desktop
        for (const auto& keys : { QKeySequence(u"Ctrl+Return"_s),
                                  QKeySequence(u"Ctrl+Enter"_s) }) {
            auto* shortcut = new QShortcut(keys, box);
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated, this, &GitView::commit_);
        }

        auto* layout = new QVBoxLayout(box);
        layout->setContentsMargins(
            GIT_VIEW_COMMIT_MARGIN,
            GIT_VIEW_COMMIT_MARGIN,
            GIT_VIEW_COMMIT_MARGIN,
            GIT_VIEW_COMMIT_MARGIN);
        layout->setSpacing(GIT_VIEW_COMMIT_SPACING);
        layout->addWidget(summaryEdit_);
        layout->addWidget(descriptionEdit_);
        layout->addWidget(commitButton_);

        return box;
    }

    // The diff, or a line in its place
    [[nodiscard]] QWidget* buildDetailArea_(QWidget* parentSplitter)
    {
        detail_ = new QStackedWidget(parentSplitter);

        message_ = new QLabel(detail_);
        message_->setAlignment(Qt::AlignCenter);
        message_->setWordWrap(true);
        message_->setForegroundRole(GIT_VIEW_MUTED_TEXT_ROLE);

        diffView_ = new GitDiffView(detail_);

        detail_->addWidget(message_);
        detail_->addWidget(diffView_);

        return detail_;
    }

    // The vault's text font, built as TextFileView builds it
    void applyFont_()
    {
        const auto& config = vault_->config();

        QFont font(config.textFontFamily(), config.textFontSize());
        font.setBold(config.textFontBold());
        font.setItalic(config.textFontItalic());

        if (font != diffView_->font()) {
            diffView_->setFont(font);
        }
    }

    // The list is made again from the status, with its signals held, and the
    // selection then settled once
    void rebuild_()
    {
        auto selected = selectedPath_;
        auto selected_row = -1;

        entries_ = repository_->status().entries;

        {
            QSignalBlocker blocker(list_);
            list_->clear();

            for (const auto& entry : entries_) {
                auto* item = new QListWidgetItem(textOf_(entry), list_);
                item->setData(
                    GitChangeDelegate::CHANGE_ROLE,
                    int(entry.change));
                item->setToolTip(entry.path.prettyQString());
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(
                    excluded_.contains(entry.path.prettyQString())
                        ? Qt::Unchecked
                        : Qt::Checked);

                if (entry.path == selected) {
                    selected_row = list_->count() - 1;
                }
            }

            if (selected_row < 0 && !entries_.isEmpty()) {
                selected_row = 0;
            }

            list_->setCurrentRow(selected_row);
        }

        allBox_->setText(
            tr("%Ln changed file(s)", nullptr, int(entries_.size())));
        updateCommitBox_();

        onSelectionChanged_(selected_row);
    }

    // A file checked or unchecked by hand
    void onItemChanged_(QListWidgetItem* item)
    {
        auto row = list_->row(item);

        if (row < 0 || row >= entries_.size()) {
            return;
        }

        if (item->checkState() == Qt::Checked) {
            excluded_.remove(entries_[row].path.prettyQString());
        } else {
            excluded_.insert(entries_[row].path.prettyQString());
        }

        updateCommitBox_();
    }

    // All checked unchecks them all; anything else checks them all
    void onAllBoxClicked_()
    {
        auto check = checkedRows_().size() != entries_.size();

        {
            QSignalBlocker blocker(list_);

            for (auto row = 0; row < list_->count(); ++row) {
                list_->item(row)->setCheckState(
                    check ? Qt::Checked : Qt::Unchecked);
            }
        }

        for (const auto& entry : entries_) {
            if (check) {
                excluded_.remove(entry.path.prettyQString());
            } else {
                excluded_.insert(entry.path.prettyQString());
            }
        }

        updateCommitBox_();
    }

    [[nodiscard]] QList<int> checkedRows_() const
    {
        QList<int> rows{};

        for (auto row = 0; row < list_->count(); ++row) {
            if (list_->item(row)->checkState() == Qt::Checked) {
                rows << row;
            }
        }

        return rows;
    }

    // The all box, and the button: its branch, and whether there's a summary
    // and a checked file to commit, with no commit already underway
    void updateCommitBox_()
    {
        auto checked = checkedRows_().size();

        allBox_->setCheckState(
            checked == 0                 ? Qt::Unchecked
            : checked == entries_.size() ? Qt::Checked
                                         : Qt::PartiallyChecked);
        allBox_->setEnabled(!entries_.isEmpty());

        const auto& branch = repository_->status().branch;
        commitButton_->setText(
            branch.name.isEmpty() ? tr("Commit")
                                  : tr("Commit to %1").arg(branch.name));

        commitButton_->setEnabled(
            !repository_->isBusy() && checked > 0 &&
            !summaryEdit_->text().trimmed().isEmpty());
    }

    void commit_()
    {
        if (!commitButton_->isEnabled()) {
            return;
        }

        QList<GitStatus::Entry> entries{};

        for (auto row : checkedRows_()) {
            entries << entries_[row];
        }

        repository_->commit(
            entries,
            summaryEdit_->text().trimmed(),
            descriptionEdit_->toPlainText().trimmed());
    }

    // The status is read again after either answer, which rebuilds the list
    void onCommitted_()
    {
        summaryEdit_->clear();
        descriptionEdit_->clear();
    }

    void onCommitFailed_(const QString& error)
    {
        showFailure_(tr("Commit failed"), tr("Nothing was committed."), error);
    }

    void onDiscardFailed_(const QString& error)
    {
        showFailure_(
            tr("Discard failed"),
            tr("The changes weren't discarded."),
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

    // Asked first, without exec, with Cancel the default. The entry is the
    // one asked about, even if a new status comes while the question is up
    void confirmDiscard_(const GitStatus::Entry& entry)
    {
        auto name = entry.path.nameQString();
        auto is_new = entry.change == GitStatus::Change::Untracked ||
                      entry.change == GitStatus::Change::Added ||
                      repository_->status().branch.unborn;

        auto* box = new QMessageBox(
            QMessageBox::Question,
            tr("Discard changes"),
            is_new ? tr("Discard “%1”?").arg(name)
                   : tr("Discard the changes to “%1”?").arg(name),
            QMessageBox::NoButton,
            this);

        box->setInformativeText(
            is_new ? tr("It isn't in the last commit, so the file itself "
                        "goes to the system trash.")
                   : tr("The changed file goes to the system trash, and the "
                        "file goes back to how it was in the last commit."));

        auto* discard =
            box->addButton(tr("Discard"), QMessageBox::DestructiveRole);
        box->setDefaultButton(box->addButton(QMessageBox::Cancel));
        box->setAttribute(Qt::WA_DeleteOnClose);

        connect(box, &QMessageBox::finished, this, [this, box, discard, entry] {
            if (box->clickedButton() != discard) {
                return;
            }

            repository_->discard(entry);
        });

        box->open();
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

    void requestOpen_(int row)
    {
        if (auto key = openableKey_(row); !key.isEmpty()) {
            emit openRequested(key);
        }
    }

    void showContextMenu_(const QPoint& position)
    {
        auto* item = list_->itemAt(position);

        if (!item) {
            return;
        }

        auto row = list_->row(item);
        auto key = openableKey_(row);
        const auto& entry = entries_[row];

        auto can_discard = !repository_->isBusy() &&
                           entry.change != GitStatus::Change::Conflicted;

        if (key.isEmpty() && !can_discard) {
            return;
        }

        QMenu menu(this);

        if (!key.isEmpty()) {
            menu.addAction(tr("Open"), this, [this, key] {
                emit openRequested(key);
            });
        }

        if (can_discard) {
            if (!key.isEmpty()) {
                menu.addSeparator();
            }

            menu.addAction(tr("Discard Changes…"), this, [this, entry] {
                confirmDiscard_(entry);
            });
        }

        menu.exec(list_->viewport()->mapToGlobal(position));
    }

    void onSelectionChanged_(int row)
    {
        if (row < 0 || row >= entries_.size()) {
            selectedPath_ = {};
            showMessage_(
                entries_.isEmpty() ? tr("No local changes") : QString{});
            return;
        }

        selectedPath_ = entries_[row].path;
        readSelectedDiff_();
    }

    // Only while the view shows: hidden, it would run git for nothing
    void readSelectedDiff_()
    {
        if (!isVisible()) {
            return;
        }

        auto row = list_->currentRow();

        if (row < 0 || row >= entries_.size()) {
            return;
        }

        repository_->readDiff(entries_[row]);
    }

    // An answer for a file no longer selected is dropped
    void onDiffRead_(const Coco::Path& path, const GitDiff::Diff& diff)
    {
        if (path != selectedPath_) {
            return;
        }

        if (diff.binary) {
            showMessage_(tr("This binary file has changed."));
            return;
        }

        if (diff.hunks.isEmpty()) {
            showMessage_(tr("This file has no changed lines to show."));
            return;
        }

        auto scroll = path == shownPath_ ? GitDiffView::Scroll::Kept
                                         : GitDiffView::Scroll::ToTop;
        shownPath_ = path;

        diffView_->showDiff(diff, scroll);
        detail_->setCurrentWidget(diffView_);
    }

    void onDiffFailed_(const Coco::Path& path, const QString& error)
    {
        if (path != selectedPath_) {
            return;
        }

        showMessage_(tr("Git couldn't show the changes:\n%1").arg(error));
    }

    void showMessage_(const QString& text)
    {
        shownPath_ = {};
        message_->setText(text);
        detail_->setCurrentWidget(message_);
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
