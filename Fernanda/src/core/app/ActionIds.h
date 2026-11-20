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

#include <QString>

namespace Fernanda::ActionIds {

using namespace Qt::StringLiterals;

// The stable ids every action is filed under in a window's registry. Named
// constants, so a mistyped id fails to compile rather than leaving a gap that
// BaseWindow::action's assert catches only in a debug build.
//
// A prefix names the thing acted on, never the menu: menus move, and ids will
// be saved in hotkey settings and shown in a command palette. See
// docs/Architecture.md, "Actions".
//
//   document.   the buffer: undo reverses in every view of the file, in every
//               window, not just the active tab
//   file.       a file in the vault
//   folder.     a folder in the vault
//   window.     one window's own chrome
//   view.       one view's presentation, not the buffer: two views of a file
//               zoom and search apart
//   vault.      the whole vault (its settings), per window
//   repository. the Git repository the vault is in, per window
//   text.       the text with keyboard focus, the editor or a field such as
//               the find bar's, where the clipboard and selection keys work
//   app.        what App owns and windows only show (AppActions); also marks
//               what comes down from App rather than being minted per window
//
// QString, not CoreConstants' constexpr auto: these key a
// QHash<QString, QAction*>, and another type would convert on each lookup.
// QString needs no QCoreApplication, so static initialization is safe

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
inline const QString VAULT_OPEN_WRITING_HISTORY = u"vault.openWritingHistory"_s;

inline const QString REPOSITORY_TOGGLE_PAGE = u"repository.togglePage"_s;

inline const QString APP_MANAGE_VAULTS = u"app.manageVaults"_s;
inline const QString APP_QUIT = u"app.quit"_s;
inline const QString APP_RESTART = u"app.restart"_s;

} // namespace Fernanda::ActionIds
