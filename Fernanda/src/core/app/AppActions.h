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

#include <QAction>
#include <QHash>
#include <QString>

namespace Fernanda {

// The actions App owns and windows only show, keyed by their ActionIds. Only
// App can perform them (a quit must go through the graceful close path, and
// the picker is App's window), so App mints each, connects it once, and hands
// the set to every window. Each window adopts all of it
// (BaseWindow::adoptAppActions_), so every shortcut works in the active
// window, and no triggered signal is relayed. See docs/Architecture.md,
// "Actions".
//
// Keyed by id, not a struct of named members, so adoption is a loop and no
// window names an action: one added in App::buildActions_ reaches every
// window. A forgotten entry is a silent absence, caught by BaseWindow::action's
// assert at menu build, not by adoptAction. A typedef until it needs to carry
// more than the pointers.
//
// One QAction in several windows is unambiguous: Qt::WindowShortcut matches
// only in the active window, and a destroyed widget drops its own association.
// Every window adopts the whole set, even what it shows nowhere: Manage Vaults
// is unreachable in a menu-less PopoutWindow until it has a shortcut, and then
// works there with no change
using AppActions = QHash<QString, QAction*>;

} // namespace Fernanda
