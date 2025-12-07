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

// Notepad scope

constexpr auto NOTEPAD_OPEN_FILE = "notepad:open_file"; /// *
constexpr auto NOTEPAD_SAVE = "notepad:save_file"; /// *
constexpr auto NOTEPAD_SAVE_AS = "notepad:save_file_as"; /// *
constexpr auto NOTEPAD_SAVE_ALL_IN_WINDOW = "notepad:save_all_in_window"; /// *
constexpr auto NOTEPAD_SAVE_ALL = "notepad:save_all"; /// *

// Notebook scope

constexpr auto NOTEBOOK_OPEN_NOTEPAD = "notebook:open_notepad"; /// *
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
constexpr auto NEW_TAB = "views:new_tab"; /// *
constexpr auto CLOSE_TAB = "views:close_tab"; /// *
constexpr auto CLOSE_TAB_EVERYWHERE = "views:close_tab_everywhere"; /// *
constexpr auto CLOSE_WINDOW_TABS = "views:close_window_tabs"; /// *
constexpr auto CLOSE_ALL_TABS = "views:close_all_tabs"; /// *

// SettingsModule scope

constexpr auto SETTINGS_DIALOG = "settings:dialog";

// WindowService scope

constexpr auto NEW_WINDOW = "windows:new"; /// *
constexpr auto WINDOWS_SET = "windows:set"; /// *, non-menu
constexpr auto RZ_WINDOWS = "windows:rz_list"; /// *, non-menu
constexpr auto CLOSE_ALL_WINDOWS = "windows:close_all"; /// *

// ColorBarModule scope

//constexpr auto RUN_COLOR_BAR = "color_bars:run"; /// *, non-menu
//constexpr auto RUN_ALL_COLOR_BARS = "color_bars:run_all"; /// *, non-menu

// FileService scope

constexpr auto OPEN_FILE_AT_PATH = "file_models:open_path"; /// *, non-menu

// TreeViewService scope

//...

} // namespace Fernanda::Commands
