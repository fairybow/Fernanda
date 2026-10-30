/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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
#include <QList>
#include <QMenu>
#include <QSizePolicy>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/PathKey.h"
#include "core/VaultEntry.h"
#include "ui/UiConstants.h"
#include "ui/widgets/MenuButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Obsidian-style vault switcher: shows the current vault's name, and pops a
// menu of the OTHER openable vaults plus a permanent "Manage Vaults" item, and
// "Open Common Vault" while there is one to open. The vault list is read live
// from a provider App threads down and rebuilt on every aboutToShow, so it
// always reflects the current recents without this widget observing a signal.
// Two things are handed in rather than owned:
//
//   - manageVaultsAction — a QAction owned and wired upstream (VaultWindow,
//     App); the switcher only displays it and forwards no signal of its own
//     for it.
//   - vaultsProvider — App's vaultEntries(), the same source the ManageVaults
//     recents column reads.
//
// The two roots are this window's vault's and the Common Vault's, which is
// empty when there's none.
//
// Choosing a vault emits vaultActivated with its root; the Sidebar relays it up
// to App's one convergence point, so switching and opening share a path
class VaultSwitcher : public QWidget
{
    Q_OBJECT

public:
    VaultSwitcher(
        const Coco::Path& currentVaultRoot,
        const Coco::Path& commonVaultRoot,
        QAction* manageVaultsAction,
        std::function<QList<VaultEntry>()> vaultsProvider,
        QWidget* parentSidebar)
        : QWidget(parentSidebar)
        , currentVaultRoot_(currentVaultRoot)
        , commonVaultRoot_(commonVaultRoot)
        , manageVaultsAction_(manageVaultsAction)
        , vaultsProvider_(std::move(vaultsProvider))
    {
        setup_();
    }

    ~VaultSwitcher() override { TRACER; }

    // Either root moved (Vault::moveRoot), or the Common Vault's changed. The
    // button shows the new name at once; the menu is rebuilt on each show
    // anyway
    void setVaultRoots(
        const Coco::Path& currentVaultRoot,
        const Coco::Path& commonVaultRoot)
    {
        currentVaultRoot_ = currentVaultRoot;
        commonVaultRoot_ = commonVaultRoot;
        showName_();
    }

signals:
    // The user picked another known vault. Carries its root — the Sidebar
    // relays this to App::onOpenOrMakeVaultRequested_
    void vaultActivated(const Coco::Path& vaultRoot);

private:
    Coco::Path currentVaultRoot_;
    Coco::Path commonVaultRoot_;
    QAction* manageVaultsAction_ = nullptr;
    std::function<QList<VaultEntry>()> vaultsProvider_;

    MenuButton* button_ =
        new MenuButton(u":/lucide/ChevronsUpDown.svg"_s, this);
    QMenu* menu_ = new QMenu(this);

    void setup_()
    {
        // Rebuilt on demand so the list never goes stale — no observing of
        // vaultsChanged needed. aboutToShow fires before each popup
        connect(menu_, &QMenu::aboutToShow, this, &VaultSwitcher::rebuildMenu_);

        // Openable entries carry their path as a tooltip; QMenu hides action
        // tooltips unless asked
        menu_->setToolTipsVisible(true);

        showName_();
        button_->setMenu(menu_);
        button_->setPopupMode(QToolButton::InstantPopup);
        // The button paints its own glyph and name (MenuButton), so
        // no tool-button style or icon size is declared; only the height is
        // ours
        button_->setFixedHeight(VAULT_SWITCHER_HEIGHT);
        button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(button_);
    }

    void showName_()
    {
        auto name = currentVaultRoot_.nameQString();
        button_->setText(name.isEmpty() ? tr("Vault") : name);
    }

    // Wipe and repopulate from the live provider. menu_->clear() deletes the
    // per-vault actions (menu-owned) but not manageVaultsAction_, which is
    // parented to the VaultWindow — so it's safe to re-add each time
    void rebuildMenu_()
    {
        menu_->clear();

        auto any_other = false;

        for (const auto& entry : vaultsProvider_()) {
            // Switcher = OTHER, openable vaults. Skip this window's own vault,
            // the Common Vault (it has an item of its own below while there is
            // one, so it never rides the recents list here), and missing/moved
            // ones — you can't switch to a folder that isn't there. Those
            // surface (with a warning and a remove action) in ManageVaults,
            // which is what the permanent "Manage Vaults" item below is for.
            //
            // This window's vault is compared as a key, not as written: a
            // record keeps the spelling it was first made with, and the vault
            // may have been opened by another
            if (entry.isCommon || entry.missing ||
                PathKey::of(entry.root) == PathKey::of(currentVaultRoot_)) {
                continue;
            }

            any_other = true;

            // Capture the root by value — the entry list is a temporary
            auto root = entry.root;
            auto action = menu_->addAction(entry.name);
            action->setToolTip(entry.root.prettyQString());

            connect(action, &QAction::triggered, this, [this, root] {
                emit vaultActivated(root);
            });
        }

        if (!any_other) {
            auto placeholder = menu_->addAction(tr("No other vaults"));
            placeholder->setEnabled(false);
        }

        menu_->addSeparator();
        menu_->addAction(manageVaultsAction_);

        // The Common Vault, while there is one, is openable as its own window
        // independent of recents — so it gets an item of its own rather than
        // riding the provider list (from which it's filtered above). Same
        // convergence as any switch: emit its root and App opens it or raises
        // the existing window. Omitted when there is no Common Vault, or this
        // window already IS it — there'd be nothing to open
        auto is_common_window =
            PathKey::of(currentVaultRoot_) == PathKey::of(commonVaultRoot_);

        if (!commonVaultRoot_.isEmpty() && !is_common_window) {
            auto open_common = menu_->addAction(tr("Open Common Vault"));
            connect(open_common, &QAction::triggered, this, [this] {
                emit vaultActivated(commonVaultRoot_);
            });
        }
    }
};

} // namespace Fernanda::Ui
