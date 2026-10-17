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
#include <QHBoxLayout>
#include <QList>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/FileRef.h"
#include "core/Vault.h"
#include "core/VaultEntry.h"
#include "ui/OpenMode.h"
#include "ui/UiConstants.h"
#include "ui/sidebar/Drawer.h"
#include "ui/sidebar/SettingsButton.h"
#include "ui/sidebar/VaultFileTree.h"
#include "ui/sidebar/VaultSwitcher.h"

namespace Fernanda::Ui {

// The vault window's left column, top to bottom:
//
//   1. VaultFileTree  — this vault's tree              [splitter pane]
//   2. Drawer(Common) — the common vault, collapsible  [splitter pane]
//   3. VaultSwitcher  — other vaults + Manage Vaults    [fixed bar, with
//      SettingsButton   — this vault's settings  the gear on its
//      right]
//
// Borrows both Vaults (project + common) from the window. It needs them to root
// each tree and to mint FileRefs from the absolute paths the trees emit — the
// tree that fired decides which Vault the file belongs to. That is the whole of
// the "which vault?" translation; the trees stay Vault-free.
//
// The switcher's vault list comes from a provider App threads down; the Sidebar
// hands it through and relays the switcher's choice back up to the window
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    // The window's actions the sidebar surfaces. Displayed or triggered here,
    // owned and connected upstream (App or the VaultWindow), so nothing the
    // sidebar shows needs a signal of its own relayed back up
    struct Actions
    {
        QAction* manageVaults = nullptr; // the switcher's menu item
        QAction* openSettings = nullptr; // the gear
    };

    Sidebar(
        Vault* vault,
        Vault* commonVault,
        const Actions& actions,
        std::function<QList<VaultEntry>()> vaultsProvider,
        QWidget* parentVaultWindow)
        : QWidget(parentVaultWindow)
        , vault_(vault)
        , commonVault_(commonVault)
        , vaultsProvider_(std::move(vaultsProvider))
    {
        setup_(actions);
    }

    ~Sidebar() override { TRACER; }

    // The two trees WorkspaceFile persists folder expansion for. Handed over at
    // construction like the sidebar splitter — WorkspaceFile reads their opaque
    // expansion blobs and observes their expansionChanged; it never drives
    // them, so this is composition-time handoff, not the reach-through that
    // would drive a component's behaviour. commonTree() is null when this
    // window IS the common vault (no common drawer — Sidebar::setup_)
    [[nodiscard]] VaultFileTree* fileTree() const { return fileTree_; }
    [[nodiscard]] VaultFileTree* commonTree() const { return commonTree_; }

    // The common drawer, whose { collapsed, height } WorkspaceFile persists —
    // the same handoff as the trees: it reads the drawer's opaque state blob
    // and observes stateChanged. Null when this window IS the common vault,
    // like commonTree()
    [[nodiscard]] Drawer* commonDrawer() const { return commonDrawer_; }

signals:
    // The one intent the window acts on. A common-vault file carries the common
    // Vault in its FileRef, so it resolves to the shared model. The mode says
    // whether to take over the active tab or force a new one — a double-click
    // and a new file replace-active, the tree's "Open in new tab" adds
    void fileActivated(const FileRef& fileRef, OpenMode mode);

    // Relayed from the VaultSwitcher: the user picked another known vault. The
    // window forwards it to App's one convergence point
    void switchVaultRequested(const Coco::Path& vaultRoot);

    // A rename or a move landed on disk. The window persists workspace now,
    // since the relocated buffer's FileRef — and so what workspace.json records
    // for its tabs — just changed but no structural tree event fired
    void entryRelocated();

private:
    Vault* vault_ = nullptr;
    Vault* commonVault_ = nullptr;
    std::function<QList<VaultEntry>()> vaultsProvider_;

    QSplitter* splitter_ = new QSplitter(Qt::Vertical, this);
    VaultFileTree* fileTree_ = nullptr;
    VaultFileTree* commonTree_ = nullptr;
    Drawer* commonDrawer_ = nullptr;
    VaultSwitcher* switcher_ = nullptr;
    SettingsButton* settingsButton_ = nullptr;

    void setup_(const Actions& actions)
    {
        auto vault_root = vault_->root();
        auto common_vault_root = commonVault_->root();

        // Main tree
        fileTree_ = new VaultFileTree(vault_->treeModel(), splitter_);
        splitter_->addWidget(fileTree_);
        wireTree_(fileTree_, vault_);

        // Common vault tree — omitted when this window IS the Common Vault. A
        // pointer test: App hands the common root's window App's own common
        // Vault rather than a second one over the same folder, so "same Vault"
        // is the single fact every consumer asks (this drawer, the
        // FileSwitcher's second listing, the tab indicator). Comparing roots
        // answers the same question, but only as long as the two stay in step
        if (vault_ != commonVault_) {
            commonTree_ =
                new VaultFileTree(commonVault_->treeModel(), splitter_);
            commonDrawer_ =
                new Drawer(tr("Common Vault"), commonTree_, splitter_);
            splitter_->addWidget(commonDrawer_);
            wireTree_(commonTree_, commonVault_);
        }

        splitter_->setChildrenCollapsible(false);
        splitter_->setStretchFactor(0, 1); // tree grows; drawer stays put
        splitter_->setHandleWidth(1);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(splitter_);

        // The switcher — a fixed-height bar below the splitter, not a pane.
        // Handed this vault's root so it can drop itself from the "other
        // vaults" menu, and the provider so its list stays live
        switcher_ = new VaultSwitcher(
            vault_root,
            common_vault_root,
            actions.manageVaults,
            vaultsProvider_,
            this);

        connect(
            switcher_,
            &VaultSwitcher::vaultActivated,
            this,
            &Sidebar::switchVaultRequested);

        // The gear. A click triggers the window's own action, the same one
        // Ctrl+, and the File menu fire, so there is one path to the dialog and
        // the sidebar knows nothing about it
        settingsButton_ = new SettingsButton(this);
        connect(
            settingsButton_,
            &SettingsButton::clicked,
            actions.openSettings,
            &QAction::trigger);

        // Obsidian's vault profile row: the switcher takes the width, the gear
        // sits at its right. The margins keep the pair's hover washes off the
        // line above and the window's edge
        auto* profile_row = new QHBoxLayout;
        profile_row->setContentsMargins(
            SIDEBAR_FOOTER_LEFT_MARGIN,
            SIDEBAR_FOOTER_TOP_MARGIN,
            SIDEBAR_FOOTER_RIGHT_MARGIN,
            SIDEBAR_FOOTER_BOTTOM_MARGIN);
        profile_row->setSpacing(0);
        profile_row->addWidget(switcher_, 1);
        profile_row->addWidget(settingsButton_, 0);

        layout->addLayout(profile_row, 0);
    }

    // Wire one tree to its owning Vault — the whole of the "which vault?"
    // translation, in one place for both the project tree and the common one.
    // The trees stay Vault-free; this is where their absolute paths become
    // Vault operations
    void wireTree_(VaultFileTree* tree, Vault* vault)
    {
        // A double-click: tag the absolute path with this Vault and re-emit as
        // a FileRef the window opens into the active tab, replacing it unless
        // that tab is pinned
        connect(
            tree,
            &VaultFileTree::fileActivated,
            this,
            [this, vault](const Coco::Path& absolute) {
                emit fileActivated(
                    vault->makeFileRef(absolute),
                    OpenMode::ReplaceActive);
            });

        // A file context-menu open carries its own placement: the menu item
        // chose NewTab / SplitRight / NewWindow. Same FileRef mapping as a
        // double-click, but the mode rides through instead of being resolved
        // downstream
        connect(
            tree,
            &VaultFileTree::openRequested,
            this,
            [this, vault](const Coco::Path& absolute, OpenMode mode) {
                emit fileActivated(vault->makeFileRef(absolute), mode);
            });

        // New file: the Vault creates it (all disk IO is the Vault's), then we
        // open it just as a double-click would — replacing the active tab
        // unless it's pinned. Empty return == create failed; the Vault already
        // warned
        connect(
            tree,
            &VaultFileTree::newFileRequested,
            this,
            [this, vault](const Coco::Path& absoluteDir) {
                auto relative = vault->createFile(absoluteDir);
                if (relative.isEmpty()) {
                    return;
                }
                emit fileActivated(
                    FileRef{ vault, relative },
                    OpenMode::ReplaceActive);
            });

        // New folder: create and stop — nothing to open. The Vault inserts the
        // row into the shared tree model before returning. TODO: expand
        // absoluteDir if it's collapsed so the new child is visible, and select
        // it
        connect(
            tree,
            &VaultFileTree::newFolderRequested,
            this,
            [vault](const Coco::Path& absoluteDir) {
                vault->createFolder(absoluteDir);
            });

        // Rename: the Vault renames on disk and re-keys any open buffer, then
        // its models retitle their own tabs. Empty return == failed (a locked
        // entry) — warn, since the user asked explicitly and nothing changed.
        // On success, nudge a workspace save
        connect(
            tree,
            &VaultFileTree::renameRequested,
            this,
            [this,
             vault](const Coco::Path& absoluteOld, const QString& newLeaf) {
                if (vault->rename(absoluteOld, newLeaf).isEmpty()) {
                    QMessageBox::warning(
                        this,
                        tr("Rename failed"),
                        tr("Could not rename \"%1\". It may be locked or open "
                           "in another program.")
                            .arg(absoluteOld.nameQString()));
                    return;
                }

                emit entryRelocated();
            });

        // Delete: the tree already confirmed. The Vault flushes, trashes, and
        // discards any open buffer at or under the entry, and those buffers'
        // views close themselves — in every window, for a common file — each
        // window saving its own workspace off the layout change, so no nudge
        // here. A false return changed nothing; the Vault logged which refusal
        // it was, so the message stays general
        connect(
            tree,
            &VaultFileTree::deleteRequested,
            this,
            [this, vault](const Coco::Path& absolute) {
                if (vault->moveToTrash(absolute)) {
                    return;
                }

                QMessageBox::warning(
                    this,
                    tr("Delete failed"),
                    tr("Could not delete \"%1\". It couldn't be saved or moved "
                       "to the trash — it may be locked or open in another "
                       "program.")
                        .arg(absolute.nameQString()));
            });

        // A move-drop: the Vault moves on disk and re-keys any open buffer
        // under the moved entry, then its models retitle their tabs — same
        // machinery as rename. Two failure messages: a name clash is the
        // common, expected one, so an advisory existence check names it
        // specifically (matching Obsidian, which refuses rather than suffixing
        // or overwriting); the Vault's authoritative empty return covers the
        // rest (locked, gone, disk error). On success, nudge a workspace save —
        // a moved buffer's FileRef, and so what workspace.json records, changed
        connect(
            tree,
            &VaultFileTree::moveRequested,
            this,
            [this, vault](
                const Coco::Path& absoluteSource,
                const Coco::Path& absoluteDestDir) {
                auto target = absoluteDestDir / absoluteSource.nameQString();
                if (target.exists()) {
                    QMessageBox::warning(
                        this,
                        tr("Couldn't move"),
                        tr("An item named \"%1\" already exists in that "
                           "folder.")
                            .arg(absoluteSource.nameQString()));
                    return;
                }

                if (vault->move(absoluteSource, absoluteDestDir).isEmpty()) {
                    QMessageBox::warning(
                        this,
                        tr("Move failed"),
                        tr("Could not move \"%1\". It may be locked or open in "
                           "another program.")
                            .arg(absoluteSource.nameQString()));
                    return;
                }

                emit entryRelocated();
            });
    }
};

} // namespace Fernanda::Ui
