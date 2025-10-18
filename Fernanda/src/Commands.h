#pragma once

// TODO: Compare with Bus.md and find what's missing
namespace Fernanda::Commands {

/// * = registered

// Application scope
constexpr auto QUIT = "application:quit";
constexpr auto ABOUT_DIALOG = "application:about_dialog"; /// *

// Workspace scope
constexpr auto NEW_TAB = "poly:new_tab";
constexpr auto NEW_NOTEBOOK = "workspace:new_notebook";
constexpr auto OPEN_NOTEBOOK = "workspace:open_notebook";
constexpr auto CLOSE_TAB = "poly:close_tab";
constexpr auto CLOSE_ALL_TABS_IN_WINDOW = "poly:close_all_tabs_in_window";
constexpr auto CLOSE_WINDOW = "workspace:close_window";

// Notepad scope
constexpr auto NOTEPAD_OPEN_FILE = "notepad:open_file";
constexpr auto NOTEPAD_SAVE = "notepad:save_file";
constexpr auto NOTEPAD_SAVE_AS = "notepad:save_file_as";
constexpr auto NOTEPAD_SAVE_ALL_IN_WINDOW = "notepad:save_all_in_window";
constexpr auto NOTEPAD_SAVE_ALL = "notepad:save_all";

// ViewService scope
constexpr auto UNDO = "views:undo";
constexpr auto REDO = "views:redo";
constexpr auto CUT = "views:cut";
constexpr auto COPY = "views:copy";
constexpr auto PASTE = "views:paste";
constexpr auto DELETE = "views:delete";
constexpr auto SELECT_ALL = "views:select_all";

// SettingsModule scope
constexpr auto SETTINGS_DIALOG = "settings:dialog";

// WindowService scope
constexpr auto NEW_WINDOW = "windows:new"; /// *
constexpr auto ACTIVE_WINDOW = "windows:active"; /// *, non-menu
constexpr auto WINDOWS_SET = "windows:set"; /// *, non-menu

// ColorBarModule scope
constexpr auto RUN_COLOR_BAR = "color_bar:run"; /// *, non-menu
constexpr auto BE_CUTE = "color_bar:be_cute"; /// *, non-menu

} // namespace Fernanda::Commands
