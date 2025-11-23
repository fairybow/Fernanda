/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

// TODO: Compare with Bus.md and find what's missing
namespace Fernanda::Commands {

/// * = registered

// Application scope

constexpr auto ABOUT_DIALOG = "application:about_dialog"; /// *

// Workspace scope

constexpr auto NEW_NOTEBOOK = "workspace:new_notebook";
constexpr auto OPEN_NOTEBOOK = "workspace:open_notebook";

// Poly: same command for all Workspace types but registered differently by
// each. These commands are meant to be called from a Workspace-agnostic service
// but with effects unique to each Workspace type

/// Closures (mark finished as implemented)

constexpr auto CLOSE_TAB = "poly:close_tab";
constexpr auto CLOSE_TAB_EVERYWHERE = "poly:close_tab_everywhere";
constexpr auto CLOSE_WINDOW_TABS = "poly:close_window_tabs";
constexpr auto CLOSE_ALL_TABS = "poly:close_all_tabs";
// Close window
constexpr auto CLOSE_ALL_WINDOWS = "poly:close_all_windows";

constexpr auto NEW_TAB = "poly:new_tab"; /// *
constexpr auto WS_TREE_VIEW_MODEL = "poly:ws_tree_view_model"; /// *, non-menu
constexpr auto WS_TREE_VIEW_ROOT_INDEX =
    "poly:ws_tree_view_root_index"; /// *, non-menu

// Notepad scope

constexpr auto NOTEPAD_OPEN_FILE = "notepad:open_file"; /// *
constexpr auto NOTEPAD_SAVE = "notepad:save_file";
constexpr auto NOTEPAD_SAVE_AS = "notepad:save_file_as";
constexpr auto NOTEPAD_SAVE_ALL_IN_WINDOW = "notepad:save_all_in_window";
constexpr auto NOTEPAD_SAVE_ALL = "notepad:save_all";

// Notebook scope

constexpr auto NOTEBOOK_OPEN_NOTEPAD = "notebook:open_notepad";
constexpr auto NOTEBOOK_IMPORT_FILE = "notebook:import_file"; /// *
constexpr auto NOTEBOOK_SAVE = "notebook:save_archive";
constexpr auto NOTEBOOK_SAVE_AS = "notebook:save_archive_as";
constexpr auto NOTEBOOK_EXPORT_FILE = "notebook:export_file";

// ViewService scope

constexpr auto UNDO = "views:undo"; /// *
constexpr auto REDO = "views:redo"; /// *
constexpr auto CUT = "views:cut"; /// *
constexpr auto COPY = "views:copy"; /// *
constexpr auto PASTE = "views:paste"; /// *
constexpr auto DEL = "views:delete"; /// *
constexpr auto SELECT_ALL = "views:select_all"; /// *

// SettingsModule scope

constexpr auto SETTINGS_DIALOG = "settings:dialog";

// WindowService scope

constexpr auto NEW_WINDOW = "windows:new"; /// *
constexpr auto WINDOWS_SET = "windows:set"; /// *, non-menu

// ColorBarModule scope

constexpr auto RUN_COLOR_BAR = "color_bars:run"; /// *, non-menu
constexpr auto RUN_ALL_COLOR_BARS = "color_bars:run_all"; /// *, non-menu

// FileService scope

constexpr auto OPEN_FILE_AT_PATH = "file_models:open_path"; /// *, non-menu

// TreeViewModule scope

//...

} // namespace Fernanda::Commands
