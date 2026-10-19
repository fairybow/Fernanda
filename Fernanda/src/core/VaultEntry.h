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

#include <QString>

#include <Coco/Path.h>

namespace Fernanda {

// One row of the vaults list. A plain value, like PathKey: nobody owns it, and
// App builds a fresh QList<VaultEntry> on demand from AppConfig's recents,
// handing it to the two surfaces that render the list — ManageVaults' recents
// column and each window's VaultSwitcher menu.
//
// Built, not stored. Only `root` is persisted (in AppConfig's recentVaults);
// `name`, `missing`, and `open` are all derived by App when it assembles the
// list, so the surfaces don't each re-derive them. `root` is the open target: a
// chosen entry lands on App::onOpenOrMakeVaultRequested_, exactly like the
// picker's own buttons.
//
// `open` is the one derived field that reads in-memory state rather than the
// disk: it's true when a window currently holds this vault (openVaults_).
// ManageVaults gates on it — a row's rename and remove-from-list are disabled
// while that vault is open — so keeping it on the entry puts the open-vault
// knowledge in one place. It's fresh per build and the surfaces rebuild on
// vaultsChanged (which fires on close as well as open), so it never goes stale
// under a live picker.
//
// `isCommon` marks the one row that IS the Common Vault. Derived by App for the
// same reason as the rest: App is the only object that holds commonVault_, so
// it is the only one that can answer by the pointer's own root rather than by a
// path a surface was separately handed. ManageVaults uses it to omit the rename
// and remove-from-list items entirely — not grey them: those two are gated on a
// CONDITION for a project vault (close it, restore the folder), and the Common
// Vault has no such condition. Its folder is resolved at launch from AppConfig,
// not from this list, and its Vault is live and windowless, so a rename would
// strand it on a dead root. VaultSwitcher already drops the Common Vault from
// its menu by root comparison and needs nothing here.
//
// No id field. A vault is its path, and the list resolves entirely by it — name
// from the folder, missing from exists(), open by path, open-target the path.
struct VaultEntry
{
    Coco::Path root{};     // the vault folder; the open target
    QString name{};        // display name — the folder's own name
    bool missing = false;  // the folder has moved or been deleted
    bool open = false;     // a window currently holds this vault
    bool isCommon = false; // this row is the Common Vault
};

} // namespace Fernanda
