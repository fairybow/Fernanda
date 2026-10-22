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

#include "core/PathKey.h"
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
//   1. VaultFileTree   — this vault's tree              [splitter pane]
//   2. Drawer          — the Common Vault, collapsible  [splitter pane]
//   3. VaultSwitcher   — other vaults + Manage Vaults   [fixed bar]
//      SettingsButton  — this vault's settings, at the bar's right
//
// Borrows both Vaults (project + common) from the window. It needs them to root
// each tree and to send each tree's file operations to the Vault that tree
// shows. The Common Vault may be null, when there is none, and then there's no
// drawer, as in the Common Vault's own window. It can change while the window
// is open (setCommonVault), and the drawer comes and goes with it. The trees
// emit absolute paths and stay Vault-free; here those paths become keys.
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

    // The Common Vault changed (App::setCommonVault_), or there's none now.
    // The drawer for the old one goes, and one for the new one comes unless
    // this window IS it; the switcher takes the new root. The window hands the
    // new drawer to WorkspaceFile (VaultWindow::setCommonVault)
    void setCommonVault(Vault* commonVault)
    {
        removeCommonDrawer_();
        commonVault_ = commonVault;
        addCommonDrawer_();
        refreshRoots_();
    }

    // The two trees WorkspaceFile persists folder expansion for. Handed over at
    // construction like the sidebar splitter, and again whenever the common
    // drawer is replaced — WorkspaceFile reads their opaque expansion blobs and
    // observes their expansionChanged; it never drives them, so this is a
    // handoff, not the reach-through that would drive a component's behaviour.
    // commonTree() is null when there is no common drawer: there's no Common
    // Vault, or this window IS it
    [[nodiscard]] VaultFileTree* fileTree() const { return fileTree_; }
    [[nodiscard]] VaultFileTree* commonTree() const { return commonTree_; }

    // The common drawer, whose { collapsed, height } WorkspaceFile persists —
    // the same handoff as the trees: it reads the drawer's opaque state blob
    // and observes stateChanged. Null when there is no common drawer, like
    // commonTree()
    [[nodiscard]] Drawer* commonDrawer() const { return commonDrawer_; }

signals:
    // The one intent the window acts on. The window decides from the key which
    // of its vaults the file is in. The mode says whether to take over the
    // active tab or force a new one — a double-click and a new file
    // replace-active, the tree's "Open in new tab" adds
    void fileActivated(const PathKey& key, OpenMode mode);

    // Relayed from the VaultSwitcher: the user picked another known vault. The
    // window forwards it to App's one convergence point
    void switchVaultRequested(const Coco::Path& vaultRoot);

    // A rename or a move landed on disk. The window persists workspace now,
    // since the relocated buffer's key — and so what workspace.json records
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
        // Main tree
        fileTree_ = new VaultFileTree(vault_->treeModel(), splitter_);
        splitter_->addWidget(fileTree_);
        wireTree_(fileTree_, vault_);

        addCommonDrawer_();

        splitter_->setChildrenCollapsible(false);
        splitter_->setStretchFactor(0, 1); // tree grows; drawer stays put
        splitter_->setHandleWidth(1);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(splitter_);

        // The switcher — a fixed-height bar below the splitter, not a pane.
        // Handed this vault's root so it can drop itself from the "other
        // vaults" menu, the Common Vault's (empty when there's none) for its
        // own item, and the provider so its list stays live
        switcher_ = new VaultSwitcher(
            vault_->root(),
            commonVault_ ? commonVault_->root() : Coco::Path{},
            actions.manageVaults,
            vaultsProvider_,
            this);

        connect(
            switcher_,
            &VaultSwitcher::vaultActivated,
            this,
            &Sidebar::switchVaultRequested);

        // This vault moved while open (Vault::moveRoot) hands the switcher
        // both roots again. The Common Vault's is connected with its drawer
        // (addCommonDrawer_)
        connect(vault_, &Vault::rootMoved, this, &Sidebar::refreshRoots_);

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

    // A pointer test: App hands the Common Vault's own window App's own
    // common Vault rather than a second one over the same folder, so "same
    // Vault" is the single fact every consumer asks (this drawer, the
    // FileSwitcher's second listing, the tab indicator). Comparing roots
    // answers the same question, but only as long as the two stay in step
    [[nodiscard]] bool hasCommonDrawer_() const
    {
        return commonVault_ && commonVault_ != vault_;
    }

    // The Common Vault's tree, in a drawer below this vault's, when there's a
    // Common Vault and this window isn't it. A new drawer starts collapsed
    void addCommonDrawer_()
    {
        if (!hasCommonDrawer_()) {
            return;
        }

        commonTree_ = new VaultFileTree(commonVault_->treeModel(), splitter_);
        commonDrawer_ =
            new Drawer(commonDrawerTitle_(), commonTree_, splitter_);
        splitter_->addWidget(commonDrawer_);
        wireTree_(commonTree_, commonVault_);

        // A toggle moves both trees under a cursor that stays put: opening
        // slides the common tree in under it, and closing leaves this vault's
        // tree there. Each re-reads its hovered row once the drawer has
        // settled
        connect(
            commonDrawer_,
            &Drawer::expansionSettled,
            fileTree_,
            &VaultFileTree::syncHover);
        connect(
            commonDrawer_,
            &Drawer::expansionSettled,
            commonTree_,
            &VaultFileTree::syncHover);

        // The Common Vault moved while open (Vault::moveRoot) hands the
        // switcher both roots again, and the drawer its title
        connect(commonVault_, &Vault::rootMoved, this, &Sidebar::refreshRoots_);
    }

    // Hidden at once, so the splitter lays out without it, and deleted when
    // control returns to the event loop, since its tree may be the one whose
    // signal led here. Its tree goes with it
    void removeCommonDrawer_()
    {
        if (!commonDrawer_) {
            return;
        }

        disconnect(
            commonVault_,
            &Vault::rootMoved,
            this,
            &Sidebar::refreshRoots_);

        commonDrawer_->hide();
        commonDrawer_->deleteLater();
        commonDrawer_ = nullptr;
        commonTree_ = nullptr;
    }

    // The Common Vault's folder name, which the drawer's header carries as its
    // title (painted or not, per COMMON_DRAWER_LABEL). A folder with no name,
    // a drive's root, falls back to "Common Vault"
    [[nodiscard]] QString commonDrawerTitle_() const
    {
        auto name = commonVault_->root().nameQString();
        return name.isEmpty() ? tr("Common Vault") : name;
    }

    // Both roots, to the switcher, and the Common Vault's name to the drawer:
    // either vault moved, or the Common Vault changed
    void refreshRoots_()
    {
        switcher_->setVaultRoots(
            vault_->root(),
            commonVault_ ? commonVault_->root() : Coco::Path{});

        if (commonDrawer_) {
            commonDrawer_->setTitle(commonDrawerTitle_());
        }
    }

    // Wire one tree to the Vault it shows, in one place for both the project
    // tree and the common one. The trees stay Vault-free; this is where their
    // absolute paths become keys and Vault operations
    void wireTree_(VaultFileTree* tree, Vault* vault)
    {
        // A double-click: re-emit as a key the window opens into the active
        // tab, replacing it unless that tab is pinned
        connect(
            tree,
            &VaultFileTree::fileActivated,
            this,
            [this](const Coco::Path& absolute) {
                emit fileActivated(
                    PathKey::of(absolute),
                    OpenMode::ReplaceActive);
            });

        // A file context-menu open carries its own placement: the menu item
        // chose NewTab / SplitRight / NewWindow. Same mapping as a
        // double-click, but the mode rides through instead of being resolved
        // downstream
        connect(
            tree,
            &VaultFileTree::openRequested,
            this,
            [this](const Coco::Path& absolute, OpenMode mode) {
                emit fileActivated(PathKey::of(absolute), mode);
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
                auto key = vault->createFile(PathKey::of(absoluteDir));
                if (key.isEmpty()) {
                    return;
                }
                emit fileActivated(key, OpenMode::ReplaceActive);
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
                vault->createFolder(PathKey::of(absoluteDir));
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
                if (vault->rename(PathKey::of(absoluteOld), newLeaf)
                        .isEmpty()) {
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
                if (vault->moveToTrash(PathKey::of(absolute))) {
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
        // a moved buffer's key, and so what workspace.json records, changed
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

                if (vault
                        ->move(
                            PathKey::of(absoluteSource),
                            PathKey::of(absoluteDestDir))
                        .isEmpty()) {
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
