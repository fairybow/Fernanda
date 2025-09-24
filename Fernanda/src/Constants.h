#pragma once

// For reused, non-translatable strings and any other reused constants

namespace Fernanda {

constexpr auto CONFIG_FILE_NAME = "Settings.ini";

namespace WorkspaceCmd {

    constexpr auto WINDOWS_R_LIST = "window_service:reverse_list";
    constexpr auto MODEL_VIEWS_COUNT = "view_service:model_views_count";

} // namespace WorkspaceCmd

namespace NotepadCmd {

    constexpr auto BASE_DIR = "notepad:base_dir";

} // namespace NotepadCmd

namespace NotebookCmd {

    //...

} // namespace NotebookCmd

namespace Commands {
    /// Rework all commands!

    // constexpr auto SetSetting = "cmd.settings_mod.workspace:set";
    // constexpr auto NewTab = "cmd.file_serv.window:new_tab";
    // constexpr auto NewWindow = "cmd.workspace.workspace:new_window";
    ///// NewNotebook
    ///// OpenNotebook
    // constexpr auto OpenFile = "cmd.file_serv.window:open_file";
    // constexpr auto CloseWindow = "cmd.workspace.workspace:close_window";
    // constexpr auto CloseAllWindows =
    //     "cmd.workspace.workspace:close_all_windows";
    // constexpr auto Quit = "cmd.workspace.application:quit";
    // constexpr auto Undo = "cmd.view_serv.workspace:undo";
    // constexpr auto Redo = "cmd.view_serv.workspace:redo";
    // constexpr auto Cut = "cmd.view_serv.workspace:cut";
    // constexpr auto Copy = "cmd.view_serv.workspace:copy";
    // constexpr auto Paste = "cmd.view_serv.workspace:paste";
    // constexpr auto Delete = "cmd.view_serv.workspace:delete";
    // constexpr auto SelectAll = "cmd.view_serv.workspace:select_all";
    // constexpr auto PreviousTab = "cmd.view_serv.window:previous_tab";
    // constexpr auto NextTab = "cmd.view_serv.window:next_tab";
    // constexpr auto PreviousWindow =
    // "cmd.window_serv.workspace:previous_window"; constexpr auto
    // ViewNextWindow = "cmd.window_serv.workspace:next_window"; constexpr auto
    // SettingsDialog = "cmd.workspace.workspace:settings_dialog"; constexpr
    // auto AboutDialog = "cmd.workspace.application:about_dialog";

    // constexpr auto NewTreeViewModel =
    //     "call.workspace.workspace:new_tree_view_model";

    ///// Rework/rethink:

    // constexpr auto CloseView = "call.view_serv.window:close_view";
    // constexpr auto CloseWindowViews =
    //     "call.view_serv.window:close_window_views";
    // constexpr auto CloseAllViews =
    // "call.view_serv.workspace:close_all_views";

    // constexpr auto NotepadSaveFile = "call.file_serv.workspace:save";
    // constexpr auto NotepadSaveFileAs = "call.file_serv.workspace:save_as";
    // constexpr auto NotepadSaveWindowFile =
    //     "call.file_serv.workspace:save_window";
    // constexpr auto NotepadSaveAllFiles = "call.file_serv.workspace:save_all";
    // constexpr auto NotepadSaveIndexesInWindow =
    //     "call.file_serv.workspace:save_indexes_in_window";

    ///// NotebookImportFile
    ///// NotebookSaveArchive
    ///// NotebookSaveArchiveAs
    ///// NotebookExportFile

    // constexpr auto NotebookRoot = "query.notebook.workspace:root";

    // constexpr auto ActiveWindow =
    // "query.window_serv.workspace:active_window"; constexpr auto WindowList =
    // "query.window_serv.workspace:list"; constexpr auto WindowSet =
    // "query.window_serv.workspace:set";

    //// This is for windows not shown (NOT minimized)
    // constexpr auto VisibleWindowCount =
    //     "query.window_serv.workspace:visible_count";

    // constexpr auto ActiveFileView =
    // "query.view_serv.window:active_file_view"; 
    // constexpr auto WindowAnyViewsOnModifiedFiles =
    //     "query.view_serv.window:has_modified_files";
    // constexpr auto WindowAnyFiles = "query.view_serv.window:has_any_files";
    // constexpr auto WorkspaceAnyViewsOnModifiedFiles =
    //     "query.view_serv.workspace:has_modified_files";
    // constexpr auto WorkspaceAnyFiles =
    //     "query.view_serv.workspace:has_any_files";

    // constexpr auto GetSetting = "query.settings_mod.workspace:get";

} // namespace Commands

} // namespace Fernanda
