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
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "core/git/GitDiff.h"
#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "ui/UiConstants.h"
#include "ui/git/GitChangeDelegate.h"
#include "ui/git/GitDiffView.h"

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
// repository's folder, and a renamed file shows where it was and where it is.
//
// The selected file's diff is on the right (GitDiffView), asked for each time
// a file is selected, and again when a new status leaves it selected, since
// the file may have changed. A file stays selected across a new status while
// it's still changed; with none selected, the first is, as in GitHub Desktop.
// In the diff's place, a line says when there's nothing to show: no changes,
// a binary file, a file whose change has no lines (an empty file, or only its
// mode changed), or git's own message when it failed.
//
// The diff is in the vault's text font, so prose reads as it does in the
// editor, following the vault's settings
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

private:
    GitRepository* repository_;
    Vault* vault_;

    QLabel* countLabel_ = nullptr;
    QListWidget* list_ = nullptr;
    QStackedWidget* detail_ = nullptr;
    QLabel* message_ = nullptr;
    GitDiffView* diffView_ = nullptr;

    // The status's entries the list was made from, in the list's order
    QList<GitStatus::Entry> entries_{};

    // The selected file's path, whose diff is wanted. Empty with none selected
    Coco::Path selectedPath_{};

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

        applyFont_();
        connect(vault_, &Vault::configChanged, this, &GitView::applyFont_);

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
        list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list_->setUniformItemSizes(true);
        list_->setItemDelegate(new GitChangeDelegate(list_));

        auto* layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(countLabel_);
        layout->addWidget(list_, 1);

        return column;
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

    // The list is made again from the status, with its selection signals
    // held, and the selection then settled once
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

                if (entry.path == selected) {
                    selected_row = list_->count() - 1;
                }
            }

            if (selected_row < 0 && !entries_.isEmpty()) {
                selected_row = 0;
            }

            list_->setCurrentRow(selected_row);
        }

        countLabel_->setText(
            tr("%Ln changed file(s)", nullptr, int(entries_.size())));

        onSelectionChanged_(selected_row);
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

        diffView_->showDiff(diff);
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
