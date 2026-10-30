/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

namespace Fernanda {

// The order of Manage Vaults' list of vaults, chosen with the button over it
// and kept in fernanda.json (AppConfig). Recent is most recently used first,
// the order the known vaults are kept in; the other two sort by folder name.
// The vault switcher's menu is always in recent order
enum class VaultOrder
{
    Recent,
    NameAscending,
    NameDescending
};

} // namespace Fernanda
