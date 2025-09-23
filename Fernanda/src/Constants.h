#pragma once

// For reused, non-translatable strings and any other reused constants

namespace Fernanda {

constexpr auto CONFIG_FILE_NAME = "Settings.ini";

} // namespace Fernanda

/*

// Commands are operations registered in their respective domains that can be
// run in any class using the Workspace's Commander. Commands take a Command
// struct argument, but it's optional
namespace Commands {

    // Scheme: type.domain.scope:action. The domain is the handler-registering
    // class. The scopes are the logical hierarchy of the program: Application,
    // Workspace, or Window.

    constexpr auto SetSetting = "cmd.settings_mod.workspace:set";

    /// Rework/rethink:

    constexpr auto NewTab = "cmd.file_serv.window:new_tab";
    constexpr auto NewWindow = "cmd.workspace.workspace:new_window";
    /// NewNotebook
    /// OpenNotebook
    constexpr auto OpenFile = "cmd.file_serv.window:open_file";
    constexpr auto CloseWindow = "cmd.workspace.workspace:close_window";
    constexpr auto CloseAllWindows =
        "cmd.workspace.workspace:close_all_windows";
    constexpr auto Quit = "cmd.workspace.application:quit";
    constexpr auto Undo = "cmd.view_serv.workspace:undo";
    constexpr auto Redo = "cmd.view_serv.workspace:redo";
    constexpr auto Cut = "cmd.view_serv.workspace:cut";
    constexpr auto Copy = "cmd.view_serv.workspace:copy";
    constexpr auto Paste = "cmd.view_serv.workspace:paste";
    constexpr auto Delete = "cmd.view_serv.workspace:delete";
    constexpr auto SelectAll = "cmd.view_serv.workspace:select_all";
    constexpr auto PreviousTab = "cmd.view_serv.window:previous_tab";
    constexpr auto NextTab = "cmd.view_serv.window:next_tab";
    constexpr auto PreviousWindow = "cmd.window_serv.workspace:previous_window";
    constexpr auto ViewNextWindow = "cmd.window_serv.workspace:next_window";
    constexpr auto SettingsDialog = "cmd.workspace.workspace:settings_dialog";
    constexpr auto AboutDialog = "cmd.workspace.application:about_dialog";

} // namespace Commands

// Calls are Commands that return a value (they can also be executed as Commands
// with no return value)
namespace Calls {

    constexpr auto NewTreeViewModel =
        "call.workspace.workspace:new_tree_view_model";

    /// Rework/rethink:

    constexpr auto CloseView = "call.view_serv.window:close_view";
    constexpr auto CloseWindowViews =
        "call.view_serv.window:close_window_views";
    constexpr auto CloseAllViews = "call.view_serv.workspace:close_all_views";

    constexpr auto NotepadSaveFile = "call.file_serv.workspace:save";
    constexpr auto NotepadSaveFileAs = "call.file_serv.workspace:save_as";
    constexpr auto NotepadSaveWindowFile =
        "call.file_serv.workspace:save_window";
    constexpr auto NotepadSaveAllFiles = "call.file_serv.workspace:save_all";
    constexpr auto NotepadSaveIndexesInWindow =
        "call.file_serv.workspace:save_indexes_in_window";

    /// NotebookImportFile
    /// NotebookSaveArchive
    /// NotebookSaveArchiveAs
    /// NotebookExportFile

} // namespace Calls

// Queries are for returning read-only values and take optional parameters
namespace Queries {

    constexpr auto NotepadBaseDir = "query.notepad.workspace:base_dir";
    constexpr auto NotebookRoot = "query.notebook.workspace:root";

    constexpr auto ActiveWindow = "query.window_serv.workspace:active_window";
    constexpr auto WindowList = "query.window_serv.workspace:list";
    constexpr auto ReverseWindowList =
        "query.window_serv.workspace:reverse_list";
    constexpr auto WindowSet = "query.window_serv.workspace:set";

    // This is for windows not shown (NOT minimized)
    constexpr auto VisibleWindowCount =
        "query.window_serv.workspace:visible_count";

    constexpr auto ActiveFileView = "query.view_serv.window:active_file_view";
    constexpr auto ViewCountForModel =
        "query.view_serv.workspace:model_view_count";
    constexpr auto WindowAnyViewsOnModifiedFiles =
        "query.view_serv.window:has_modified_files";
    constexpr auto WindowAnyFiles = "query.view_serv.window:has_any_files";
    constexpr auto WorkspaceAnyViewsOnModifiedFiles =
        "query.view_serv.workspace:has_modified_files";
    constexpr auto WorkspaceAnyFiles =
        "query.view_serv.workspace:has_any_files";

    constexpr auto GetSetting = "query.settings_mod.workspace:get";

} // namespace Queries

*/
