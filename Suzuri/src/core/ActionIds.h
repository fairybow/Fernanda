/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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

namespace Suzuri::ActionIds {

using namespace Qt::StringLiterals;

// The stable ids every action is filed under in a window's registry. Named
// constants rather than loose literals because a mistyped id is otherwise a
// silent runtime gap — BaseWindow::action asserts on a miss, but only in a
// debug build; here the compiler catches it either way.
//
// THE NAMING RULE: the prefix names the thing acted on, never the menu the
// action currently sits in. A menu can move — a View-menu item becoming a
// Window-menu item shouldn't invalidate a persisted hotkey or a palette entry —
// and these strings are exactly what gets persisted in the hotkey config and
// shown in a future command palette. So:
//
//   document.   the buffer. Undo routes through the model's shared prime, so
//               it reverses in every view of that file, in every window —
//               it is not "the active tab's undo" and must not be named as
//               though it were
//   file.       a file on disk in the vault
//   folder.     a directory in the vault
//   window.     one window's own chrome
//   view.       one view's presentation, per-view and not the buffer: zoom is
//               view-local (two views of one PDF zoom independently), so it
//               is named for the view, not document. A search is the same:
//               each view has its own. Like document., a menu can move it
//               without invalidating a persisted hotkey
//   vault.      the vault as a whole, beyond any one file or folder in it — its
//               settings. Per-window like file., since each window edits its
//               own vault
//   text.       the text that has keyboard focus: the editor, or a text
//               field such as the find bar's. The clipboard and selection
//               commands act there, where the keys for them already work, so
//               from the menu they reach the find field as readily as the
//               editor. Nothing else has text to act on
//   app.        App owns the behaviour and windows only display it — the
//               AppActions set. This prefix doubles as the marker for what
//               travels down from App rather than being minted per window
//
// QString rather than the constexpr auto used in CoreConstants: these are
// QHash<QString, QAction*> keys, hit at every menu build and every future
// palette lookup, and a char/QLatin1 constant would convert on each one.
// QString has no dependency on QCoreApplication, so namespace-scope static
// initialization is safe

inline const QString DOCUMENT_UNDO = u"document.undo"_s;
inline const QString DOCUMENT_REDO = u"document.redo"_s;

inline const QString TEXT_CUT = u"text.cut"_s;
inline const QString TEXT_COPY = u"text.copy"_s;
inline const QString TEXT_PASTE = u"text.paste"_s;
inline const QString TEXT_DELETE = u"text.delete"_s;
inline const QString TEXT_SELECT_ALL = u"text.selectAll"_s;

inline const QString FILE_NEW = u"file.new"_s;
inline const QString FILE_OPEN = u"file.open"_s;
inline const QString FOLDER_NEW = u"folder.new"_s;

inline const QString WINDOW_TOGGLE_MENU_BAR = u"window.toggleMenuBar"_s;

inline const QString VIEW_ZOOM_IN = u"view.zoomIn"_s;
inline const QString VIEW_ZOOM_OUT = u"view.zoomOut"_s;
inline const QString VIEW_ZOOM_RESET = u"view.zoomReset"_s;

inline const QString VIEW_FIND = u"view.find"_s;
inline const QString VIEW_REPLACE = u"view.replace"_s;
inline const QString VIEW_FIND_NEXT = u"view.findNext"_s;
inline const QString VIEW_FIND_PREVIOUS = u"view.findPrevious"_s;

inline const QString VAULT_OPEN_SETTINGS = u"vault.openSettings"_s;

inline const QString APP_MANAGE_VAULTS = u"app.manageVaults"_s;
inline const QString APP_QUIT = u"app.quit"_s;
inline const QString APP_RESTART = u"app.restart"_s;

} // namespace Suzuri::ActionIds
