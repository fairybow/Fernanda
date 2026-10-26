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

#include <QFont>
#include <QLabel>
#include <QShowEvent>
#include <QStackedLayout>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "core/git/GitDiff.h"
#include "core/git/GitRepository.h"
#include "core/git/GitStatus.h"
#include "ui/UiConstants.h"
#include "ui/git/GitDiffView.h"
#include "ui/tabs/PageIcon.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// A tab showing one changed file's diff (GitDiffView), opened from the
// sidebar's Git page. It's its own kind of page, not a file view: a diff is
// read from git and never edited, and the file it's of keeps its one buffer
// in the store, which its own tabs show. Its tab carries the file's name and
// the Git mark (GIT_DIFF_PAGE_ICON_PATH), so it isn't taken for the file.
// Like a file's, it opens over the active tab unless that one is pinned, and
// can be pinned, split, or moved to a pop-out. It isn't saved with the
// workspace.
//
// The diff is read while the page shows, and again when the repository's
// status changes while it shows, since the file may have changed; a hidden
// page reads nothing. The file is known by its path, and its entry (how it
// changed) is taken from each new status. When the status no longer lists it,
// committed or discarded, a line says there are no changes in the diff's
// place, and the tab stays until it's closed. A line also stands in for a
// binary file, a change with no lines (an empty file, or only its mode), or
// git's own message when it fails.
//
// The diff is in the vault's text font, so prose reads as it does in the
// editor, following the vault's settings
class GitDiffPage : public QWidget
{
    Q_OBJECT

public:
    // repository is the window's, and vault the window's own, whose settings
    // give the diff its font. Pop-outs close with their window, so a page in
    // one never outlives either
    GitDiffPage(
        GitRepository* repository,
        Vault* vault,
        const GitStatus::Entry& entry)
        : QWidget(nullptr)
        , repository_(repository)
        , vault_(vault)
        , entry_(entry)
    {
        setup_();
    }

    ~GitDiffPage() override { TRACER; }

protected:
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        readDiff_();
    }

private:
    GitRepository* repository_;
    Vault* vault_;

    // The file shown, as the last status listed it. Its path stays the same
    GitStatus::Entry entry_;

    // Whether the last status listed the file
    bool listed_ = true;

    QStackedLayout* stack_ = nullptr;
    QLabel* message_ = nullptr;
    GitDiffView* diffView_ = nullptr;

    void setup_()
    {
        setWindowTitle(entry_.path.nameQString());
        setToolTip(entry_.path.prettyQString());
        setPageIconPath(this, QString::fromLatin1(GIT_DIFF_PAGE_ICON_PATH));

        message_ = new QLabel(this);
        message_->setAlignment(Qt::AlignCenter);
        message_->setWordWrap(true);
        message_->setForegroundRole(GIT_DIFF_MUTED_TEXT_ROLE);

        diffView_ = new GitDiffView(this);

        stack_ = new QStackedLayout(this);
        stack_->addWidget(diffView_);
        stack_->addWidget(message_);

        setFocusProxy(diffView_);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitDiffPage::onStatusChanged_);

        connect(
            repository_,
            &GitRepository::diffRead,
            this,
            &GitDiffPage::onDiffRead_);

        connect(
            repository_,
            &GitRepository::diffFailed,
            this,
            &GitDiffPage::onDiffFailed_);

        applyFont_();
        connect(vault_, &Vault::configChanged, this, &GitDiffPage::applyFont_);
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

    // The file's entry from the new status, then its diff, if the page
    // shows. A status for a repository lost or not yet read lists nothing,
    // which reads as no changes
    void onStatusChanged_()
    {
        listed_ = false;

        for (const auto& entry : repository_->status().entries) {
            if (entry.path == entry_.path) {
                entry_ = entry;
                listed_ = true;
                break;
            }
        }

        if (!listed_) {
            showMessage_(tr("No changes"));
            return;
        }

        readDiff_();
    }

    // Only while the page shows: hidden, it would run git for nothing
    void readDiff_()
    {
        if (isVisible() && listed_) {
            repository_->readDiff(entry_);
        }
    }

    // Every page hears every diff read; this one takes its own file's
    void onDiffRead_(const Coco::Path& path, const GitDiff::Diff& diff)
    {
        if (path != entry_.path) {
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
        stack_->setCurrentWidget(diffView_);
    }

    void onDiffFailed_(const Coco::Path& path, const QString& error)
    {
        if (path != entry_.path) {
            return;
        }

        showMessage_(tr("Git couldn't show the changes:\n%1").arg(error));
    }

    void showMessage_(const QString& text)
    {
        message_->setText(text);
        stack_->setCurrentWidget(message_);
    }
};

} // namespace Fernanda::Ui
