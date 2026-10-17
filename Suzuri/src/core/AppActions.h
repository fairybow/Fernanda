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

#include <QAction>
#include <QHash>
#include <QString>

namespace Fernanda {

// The actions App owns and windows only display, keyed by the same ids the
// windows file them under (ActionIds). App is the only object that can perform
// any of these — quitting must route through the graceful close path, and the
// picker is App's window — so App mints each one, connects it exactly once, and
// hands this set to every window it creates. The window adopts the whole set
// into its own registry (BaseWindow::adoptAppActions_), which window-adds each
// so its shortcut is live whenever that window is active. Nothing here is owned
// by a window, and no triggered signal is ever relayed upward.
//
// Id-keyed rather than a struct of named members so adoption is a loop and
// nothing in the window layer names an individual action: a new App-owned
// action is added in App::buildActions_ and every window picks it up. The cost
// is that a forgotten entry is a silent absence rather than a null pointer —
// the assert that catches it is BaseWindow::action at menu-build time, not
// adoptAction. A typedef today; it becomes a real type the moment it needs to
// carry anything besides the pointers.
//
// A shared QAction associated with several top-level windows is unambiguous:
// Qt::WindowShortcut matching tests whether an associated widget lives in the
// ACTIVE window, so exactly one association can match at a time. Widget
// destruction deregisters its own association, so a closed window leaves the
// action intact for the rest.
//
// Both window types adopt the WHOLE set, including actions they show no
// surface for. A PopoutWindow has no menu, so Manage Vaults is invisible there
// and — having no shortcut today — unreachable; adopting it anyway costs one
// held pointer and means the day it gains a shortcut it works everywhere with
// no window type revisited
using AppActions = QHash<QString, QAction*>;

} // namespace Fernanda
