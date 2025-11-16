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

constexpr auto QUIT = "application:quit";
constexpr auto ABOUT_DIALOG = "application:about_dialog"; /// *

// Workspace scope

constexpr auto NEW_NOTEBOOK = "workspace:new_notebook";
constexpr auto OPEN_NOTEBOOK = "workspace:open_notebook";
constexpr auto CLOSE_WINDOW = "workspace:close_window";

// Poly: same command for all Workspace types but registered differently by
// each. These commands are meant to be called from a Workspace-agnostic service
// but with effects unique to each Workspace type

constexpr auto NEW_TAB = "poly:new_tab"; /// *
constexpr auto CLOSE_TAB = "poly:close_tab";
constexpr auto CLOSE_ALL_TABS_IN_WINDOW = "poly:close_all_tabs_in_window";
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
constexpr auto REMOVE_VIEW = "views:remove"; /// *, non-menu
constexpr auto MODEL_VIEW_COUNT = "views:model_view_count"; /// *, non-menu
constexpr auto MODEL_AT = "views:model_at"; /// *, non-menu

// SettingsModule scope

constexpr auto SET_SETTINGS_OVERRIDE = "settings:set_override"; /// *, non-menu
constexpr auto SETTINGS_DIALOG = "settings:dialog";

// WindowService scope

constexpr auto NEW_WINDOW = "windows:new"; /// *
constexpr auto ACTIVE_WINDOW = "windows:active"; /// *, non-menu
constexpr auto WINDOWS_SET = "windows:set"; /// *, non-menu

// ColorBarModule scope

constexpr auto RUN_COLOR_BAR = "color_bars:run"; /// *, non-menu
constexpr auto RUN_ALL_COLOR_BARS = "color_bars:run_all"; /// *, non-menu
constexpr auto BE_CUTE = "color_bars:be_cute"; /// *, non-menu

// FileService scope

constexpr auto OPEN_FILE_AT_PATH = "file_models:open_path"; /// *, non-menu
constexpr auto NEW_TXT_FILE = "file_models:new_txt"; /// *, non-menu
constexpr auto SET_PATH_TITLE_OVERRIDE =
    "file_models:set_path_title_override"; /// *, non-menu
constexpr auto DESTROY_MODEL = "file_models:destroy"; /// *, non-menu

// TreeViewModule scope

constexpr auto RENAME_TREE_VIEW_INDEX = "tree_views:rename_index"; /// *

} // namespace Fernanda::Commands
