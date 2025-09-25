#pragma once

// For reused, non-translatable strings and any other reused constants

namespace Fernanda {

constexpr auto CONFIG_FILE_NAME = "Settings.ini";

// Commands that apply to any Workspace
namespace WorkspaceCmd {

    constexpr auto WINDOWS_ACTIVE = "windows:active";
    constexpr auto WINDOWS_LIST = "windows:list";
    constexpr auto WINDOWS_R_LIST = "windows:reverse_list";
    constexpr auto WINDOWS_SET = "windows:set";

    constexpr auto MODEL_VIEWS_COUNT = "views:model_views_count";
    constexpr auto UNDO = "views:undo";
    constexpr auto REDO = "views:redo";
    constexpr auto CUT = "views:cut";
    constexpr auto COPY = "views:copy";
    constexpr auto PASTE = "views:paste";
    constexpr auto DELETE = "views:delete";
    constexpr auto SELECT_ALL = "views:select_all";

    // Todo: Note why this is in Workspace and not WindowService!
    constexpr auto NEW_WINDOW = "workspace:new_window";

    constexpr auto SETTINGS_GET = "settings:get";
    constexpr auto SETTINGS_SET = "settings:set";
    constexpr auto SETTINGS_DIALOG = "settings:dialog";

} // namespace WorkspaceCmd

// Commands whose functionality depends on the workspace type
namespace PolyCmd {

    /// Not clear this will be here or only ever called by objects that care
    /// about Notepad?
    constexpr auto BASE_DIR = "notepad:base_dir";

    /// Called by objects that do not know about the workspace type and don't
    /// need to
    constexpr auto NEW_TAB = "workspace_subclasses:new_tab";
    constexpr auto NEW_TREE_VIEW_MODEL =
        "workspace_subclasses:new_tree_view_model";

} // namespace PolyCmd

/// The following old commands need reimplementing (unless they were unused
/// before). We should also determine if any will be Notepad or Notebook
/// specific, like NewTab):

///// NewNotebook
///// OpenNotebook
// constexpr auto OpenFile = "cmd.file_serv.window:open_file";
// constexpr auto CloseWindow = "cmd.workspace.workspace:close_window";
// constexpr auto CloseAllWindows =
//     "cmd.workspace.workspace:close_all_windows";
// constexpr auto Quit = "cmd.workspace.application:quit";
// constexpr auto PreviousTab = "cmd.view_serv.window:previous_tab";
// constexpr auto NextTab = "cmd.view_serv.window:next_tab";
// constexpr auto PreviousWindow =
// "cmd.window_serv.workspace:previous_window"; constexpr auto
// ViewNextWindow = "cmd.window_serv.workspace:next_window";
// constexpr
// auto AboutDialog = "cmd.workspace.application:about_dialog";
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
//// This is for windows not shown (NOT minimized)
//// !!!! Change to WINDOWS_UNSHOWN_COUNT
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

} // namespace Fernanda
