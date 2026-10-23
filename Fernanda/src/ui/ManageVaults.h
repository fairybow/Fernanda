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

#include <QAction>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/VaultEntry.h"
#include "ui/AboutPanel.h"
#include "ui/NewVaultDialog.h"
#include "ui/RenameDialog.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"

namespace Fernanda::Ui {

class ManageVaults : public QWidget
{
    Q_OBJECT

public:
    // Two live providers, not cached values: ManageVaults can stay open while
    // vaults are opened from elsewhere, so it reads App's current state at
    // click/refresh time. startDirProvider yields the create/open MRU dir;
    // vaultsProvider yields the MRO vaults list for the recents column.
    // vaultRenamer renames a vault's folder in place, from its old root to its
    // new one, and says whether it did: App's, so every vault's rename goes
    // through App
    ManageVaults(
        std::function<Coco::Path()> startDirProvider,
        std::function<QList<VaultEntry>()> vaultsProvider,
        std::function<bool(const Coco::Path&, const Coco::Path&)> vaultRenamer,
        QAction* quitAction)
        : QWidget(nullptr)
        , startDirProvider_(std::move(startDirProvider))
        , vaultsProvider_(std::move(vaultsProvider))
        , vaultRenamer_(std::move(vaultRenamer))
    {
        setup_(quitAction);
    }

    ~ManageVaults() override { TRACER; }

    // Rebuild the recents column from the live provider. App connects this to
    // its vaultsChanged signal, so a picker left open reflects a vault opened,
    // closed, removed, or renamed under it. Cheap and wholesale — the list is
    // short
    void refreshRecents()
    {
        recentsList_->clear();

        const auto entries = vaultsProvider_();

        if (entries.isEmpty()) {
            auto placeholder = new QListWidgetItem(tr("No recent vaults yet."));
            placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
            recentsList_->addItem(placeholder);
            return;
        }

        for (const auto& entry : entries) {
            recentsList_->addItem(buildRecentItem_(entry));
        }
    }

signals:
    void openOrMakeVaultRequested(const Coco::Path& vaultRoot);

    // Remove-from-list: a registry edit, not a filesystem one — the folder is
    // untouched, only the recents entry is dropped. App owns recents, so this
    // routes there rather than ManageVaults editing AppConfig
    void forgetVaultRequested(const Coco::Path& vaultRoot);

    // Make the vault at vaultRoot the Common Vault, or have none. App owns the
    // choice (it's saved in fernanda.json) and every window's drawer, so these
    // route there
    void commonVaultSetRequested(const Coco::Path& vaultRoot);
    void commonVaultUnsetRequested();

private:
    // Yields the directory the create/open dialogs should start in — the
    // create/open MRU dir, or Documents on first run. A live provider, not a
    // cached value: ManageVaults can stay open while vaults are opened from
    // elsewhere, so it reads App's current value at click time rather than
    // snapshotting a stale one at construction
    std::function<Coco::Path()> startDirProvider_;

    // Yields the MRO vaults list for the recents column. Same live-read
    // rationale as startDirProvider_ — App owns the data, this reads it fresh
    std::function<QList<VaultEntry>()> vaultsProvider_;

    // Renames a vault's folder. App's, since App owns the vaults, open or not,
    // and everything that follows a folder when it's renamed
    std::function<bool(const Coco::Path&, const Coco::Path&)> vaultRenamer_;

    QListWidget* recentsList_ = nullptr;

    // Item-data roles for a recents row: the open target (at UserRole) plus the
    // three derived flags the context menu gates on. Stashed at build time and
    // refreshed whenever refreshRecents rebuilds the list (on vaultsChanged),
    // so a right-click reads current state without re-scanning the provider
    static constexpr int RootRole_ = Qt::UserRole;
    static constexpr int MissingRole_ = Qt::UserRole + 1;
    static constexpr int OpenRole_ = Qt::UserRole + 2;
    static constexpr int CommonRole_ = Qt::UserRole + 3;

    void setup_(QAction* quitAction)
    {
        // TODO: Should all instances of delete on close be applied in the class
        // itself or from the outside?
        setAttribute(Qt::WA_DeleteOnClose);

        setupUi_();

        // A fixed size, but never less than the layout needs: a fixed size
        // smaller than that squeezes the columns, and a label with a picture
        // in it (the About panel's icon) is cut off rather than refused
        setFixedSize(QSize(MANAGE_VAULTS_WIDTH, MANAGE_VAULTS_HEIGHT)
                         .expandedTo(minimumSizeHint()));

        addAction(quitAction);
    }

    void setupUi_()
    {
        // Left column is icon, version, and buttons. Right side is the Recent
        // Vaults list

        auto root_layout = new QHBoxLayout(this);
        root_layout->setContentsMargins(48, 48, 48, 48);
        root_layout->setSpacing(48);

        root_layout->addLayout(buildMainColumn_(), 1);
        root_layout->addLayout(buildRecentsColumn_(), 1);
    }

    QVBoxLayout* buildMainColumn_()
    {
        auto column_layout = new QVBoxLayout;
        column_layout->setSpacing(12);
        column_layout->addStretch();

        column_layout->addWidget(new AboutPanel(this));

        column_layout->addSpacing(24);

        column_layout->addWidget(
            Ui::buildPushButton(
                tr("Create new vault"),
                this,
                &ManageVaults::onNewVaultClicked_));

        column_layout->addWidget(
            Ui::buildPushButton(
                tr("Open folder as vault"),
                this,
                &ManageVaults::onOpenFolderAsVaultClicked_));

        column_layout->addStretch();

        return column_layout;
    }

    QVBoxLayout* buildRecentsColumn_()
    {
        // The recents list reads App's vaults list via the provider. Entries
        // can point at folders that have since moved or been deleted, so
        // buildRecentItem_ marks a missing state (an alert icon, disabled row)
        // rather than assuming every path resolves
        auto column_layout = new QVBoxLayout;
        column_layout->setSpacing(12);

        column_layout->addWidget(new QLabel(tr("Recent vaults"), this));

        recentsList_ = new QListWidget(this);

        // Activation (double-click / Enter) opens; a stray single click while
        // scanning the list shouldn't launch a window. Single-click-to-open is
        // a one-line switch to itemClicked if we want Obsidian's exact feel
        connect(
            recentsList_,
            &QListWidget::itemActivated,
            this,
            &ManageVaults::onRecentActivated_);

        // Per-row actions live in a right-click menu, Obsidian-style, so the
        // fixed layout is untouched. A disabled (missing) row still yields an
        // item to itemAt and still emits this, so remove-from-list reaches a
        // moved/deleted vault — which is exactly when you want it
        recentsList_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(
            recentsList_,
            &QWidget::customContextMenuRequested,
            this,
            &ManageVaults::onRecentsContextMenu_);

        column_layout->addWidget(recentsList_, 1);

        refreshRecents();

        return column_layout;
    }

    // One row. Text is the vault name; the open target and the three gate flags
    // ride in item data. A missing folder gets the standard warning icon and is
    // disabled, so it can't activate — the teeth of the missing state
    QListWidgetItem* buildRecentItem_(const VaultEntry& entry)
    {
        auto item = new QListWidgetItem(entry.name);
        item->setData(RootRole_, entry.root.toQString());
        item->setData(MissingRole_, entry.missing);
        item->setData(OpenRole_, entry.open);
        item->setData(CommonRole_, entry.isCommon);

        if (entry.missing) {
            item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
            item->setToolTip(
                tr("Folder not found — %1").arg(entry.root.prettyQString()));
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        } else {
            item->setToolTip(entry.root.prettyQString());
        }

        return item;
    }

    void onNewVaultClicked_()
    {
        NewVaultDialog dialog(this, startDirProvider_());

        // The dialog's validation is advisory; mkdir is authoritative. The
        // target can appear between the two, so re-exec on failure with the
        // error inline and the user's input preserved
        while (dialog.exec() == QDialog::Accepted) {
            auto root = dialog.vaultRoot();

            if (Coco::mkdir(root)) {
                emit openOrMakeVaultRequested(root);
                return;
            }

            dialog.setError(tr("Could not create a folder there."));
        }
    }

    void onOpenFolderAsVaultClicked_()
    {
        auto dir =
            Coco::getDir(this, tr("Open folder as vault"), startDirProvider_());
        if (dir.isEmpty()) {
            return; // cancelled
        }

        emit openOrMakeVaultRequested(dir);
    }

    void onRecentActivated_(QListWidgetItem* item)
    {
        // Disabled (missing) items don't activate, but re-stat anyway: the list
        // can be stale, and onOpenOrMakeVaultRequested_ requires the root to
        // exist. This is the same signal the picker's own buttons emit, so
        // every source converges on App's one slot
        auto root = Coco::Path(item->data(RootRole_).toString());
        if (!root.exists()) {
            return;
        }

        emit openOrMakeVaultRequested(root);
    }

    // Build the per-row menu on demand from the item's stashed flags. A
    // condition that gates an item greys it: rename, reveal, and Set as Common
    // Vault grey on a missing folder, and remove-from-list while the vault is
    // open.
    //
    // The Common Vault's row offers Unset Common Vault in place of Set as
    // Common Vault and remove-from-list: it can be removed once it's unset, on
    // the next right-click. Gating is enforced here AND upstream (App's guarded
    // slots, AppConfig refusing to forget the Common Vault): the UI shows
    // what's possible, and what shouldn't slip through is refused there
    void onRecentsContextMenu_(const QPoint& pos)
    {
        auto item = recentsList_->itemAt(pos);
        if (!item) {
            return;
        }

        // The empty-recents placeholder carries no path — no menu for it
        auto root_string = item->data(RootRole_).toString();
        if (root_string.isEmpty()) {
            return;
        }

        auto root = Coco::Path(root_string);
        auto missing = item->data(MissingRole_).toBool();
        auto open = item->data(OpenRole_).toBool();
        auto is_common = item->data(CommonRole_).toBool();

        QMenu menu(this);

        // Rename is a folder op, so there must be a folder to rename. An open
        // vault, or the Common Vault, follows its folder to the new name
        // (App::followVaults_)
        auto rename_action = menu.addAction(tr("Rename…"));
        rename_action->setEnabled(!missing);
        connect(rename_action, &QAction::triggered, this, [this, root] {
            onRenameVault_(root);
        });

        // Reveal is purely local — no registry, no App — and only meaningful
        // for a folder that's actually there
        auto reveal_action = menu.addAction(tr("Reveal in file explorer"));
        reveal_action->setEnabled(!missing);
        connect(reveal_action, &QAction::triggered, this, [root] {
            QDesktopServices::openUrl(QUrl::fromLocalFile(root.toQString()));
        });

        menu.addSeparator();

        if (is_common) {
            auto unset_action = menu.addAction(tr("Unset Common Vault"));
            connect(unset_action, &QAction::triggered, this, [this] {
                emit commonVaultUnsetRequested();
            });
        } else {
            auto set_action = menu.addAction(tr("Set as Common Vault"));
            set_action->setEnabled(!missing);
            connect(set_action, &QAction::triggered, this, [this, root] {
                emit commonVaultSetRequested(root);
            });

            // Remove-from-list drops the recents entry only (files untouched).
            // Disabled on an open vault — an on-screen vault shouldn't vanish
            // from the list under itself — but enabled on a missing one, the
            // case it's most for
            menu.addSeparator();

            auto remove_action = menu.addAction(tr("Remove from list"));
            remove_action->setEnabled(!open);
            connect(remove_action, &QAction::triggered, this, [this, root] {
                emit forgetVaultRequested(root);
            });
        }

        menu.exec(recentsList_->viewport()->mapToGlobal(pos));
    }

    // Gather a new name and have App rename the folder. The dialog checks the
    // name as it's typed, by the same rules as a new vault's (UserTextEntry),
    // and against the folders beside it; like those, the checks are advisory,
    // and the rename is the authority. On success the list refreshes with the
    // new name, through vaultsChanged; on failure the folder is unchanged, so
    // we just report and leave the list alone
    void onRenameVault_(const Coco::Path& oldRoot)
    {
        auto name = oldRoot.nameQString();

        RenameDialog
            dialog(this, tr("Rename vault"), oldRoot.parent(), name, name, {});

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        if (!vaultRenamer_(oldRoot, oldRoot.parent() / dialog.newLeaf())) {
            QMessageBox::warning(
                this,
                tr("Rename vault"),
                tr("Couldn't rename the folder. It may be in use or "
                   "read-only."));
        }
    }
};

} // namespace Fernanda::Ui
