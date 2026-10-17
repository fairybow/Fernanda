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

namespace Fernanda::Ui {

// How a file-open request should place its view. A double-click in a
// VaultFileTree — and creating a new file — opens with ReplaceActive: the file
// takes over the active tab, Obsidian-style, unless that tab is pinned, in
// which case it lands in a new one. The file context menu's items each name
// their own placement instead: NewTab always adds a tab to the active leaf;
// SplitRight splits the active leaf to the right and opens there; NewWindow
// opens in a fresh pop-out. None of those three ever replace the active tab
enum class OpenMode
{
    ReplaceActive,
    NewTab,
    SplitRight,
    NewWindow
};

} // namespace Fernanda::Ui
