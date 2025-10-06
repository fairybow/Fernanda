# Command Reference

### General Commands

Commands registered in `Workspace` or `Application` level. Available to all workspace types.

| Command ID | Handler Location | Description |
|------------|-----------------|-------------|
| `workspace:new_window` | Workspace | Creates new window in current workspace |
| `workspace:new_notebook` | Application | Creates and opens new notebook workspace |
| `workspace:open_notebook` | Application | Opens existing notebook workspace |
| `workspace:close_window` | Workspace | Closes window (delegates to close acceptor) |
| `application:quit` | Application | Quits entire application |
| `workspace:about_dialog` | Workspace | Shows About dialog |
| `settings:dialog` | SettingsModule | Opens settings dialog |
| `settings:get` | SettingsModule | Retrieves setting value |
| `settings:set` | SettingsModule | Sets setting value |
| `windows:active` | WindowService | Returns active window |
| `windows:list` | WindowService | Returns ordered window list |
| `windows:reverse_list` | WindowService | Returns reverse-ordered list |
| `windows:set` | WindowService | Returns unordered window set |

### PolyCommands

Same command ID, different implementations per workspace type. Registered in both `Notepad` and `Notebook`.

| Command ID | Description | Notepad Behavior | Notebook Behavior |
|------------|-------------|------------------|-------------------|
| `poly:new_tab` | Create new tab | Creates new text file | Creates new text file in archive |
| `poly:close_tab` | Close tab | Prompts if file modified & single view | May not prompt (temp persists) |
| `poly:close_all_tabs_in_window` | Close all tabs in window | Checks each file for modifications | Checks if archive modified |
| `poly:new_tree_view_model` | Create tree view model | Returns QFileSystemModel | Returns ArchiveModel |

### Notepad Commands

Commands only registered and handled by `Notepad` workspace.

| Command ID | Description |
|------------|-------------|
| `notepad:open_file` | Open file from OS filesystem |
| `notepad:save_file` | Save current file |
| `notepad:save_file_as` | Save current file with new path |
| `notepad:save_all_in_window` | Save all modified files in window |
| `notepad:save_all` | Save all modified files in workspace |

### Notebook Commands

Commands only registered and handled by `Notebook` workspace.

| Command ID | Description |
|------------|-------------|
| `notebook:import_file` | Import file into archive |
| `notebook:open_notepad` | Switch to/activate Notepad workspace |
| `notebook:save_archive` | Save entire archive to disk |
| `notebook:save_archive_as` | Save archive with new path |
| `notebook:export_file` | Export file from archive to OS filesystem |

### View/Edit Commands

Commands registered in `ViewService`. Work on active view/file.

| Command ID | Description |
|------------|-------------|
| `views:undo` | Undo last edit |
| `views:redo` | Redo last undone edit |
| `views:cut` | Cut selection |
| `views:copy` | Copy selection |
| `views:paste` | Paste clipboard |
| `views:delete` | Delete selection |
| `views:select_all` | Select all text |
| `views:model_views_count` | Query view count for model |

### Future Commands

Commands for future implementation:

| Command ID | Handler Location | Description |
|------------|------------------|-------------|
| `views:previous_tab` | ViewService | Activate previous tab in window |
| `views:next_tab` | ViewService | Activate next tab in window |
| `windows:previous_window` | WindowService | Activate previous window |
| `windows:next_window` | WindowService | Activate next window |
| `workspace:close_all_tabs` | Workspace | Close all tabs across all windows |
| `workspace:close_all_windows` | Workspace | Close all windows in workspace |
| `views:find` | ViewService | Open find dialog |
| `views:replace` | ViewService | Open replace dialog |
| `views:goto_line` | ViewService | Open goto line dialog |
| `views:toggle_word_wrap` | ViewService | Toggle word wrap for active view |
| `views:toggle_line_numbers` | ViewService | Toggle line numbers for active view |
| `views:zoom_in` | ViewService | Increase font size |
| `views:zoom_out` | ViewService | Decrease font size |
| `views:reset_zoom` | ViewService | Reset font size to default |

---

## Toggle States

Actions marked `[Toggle]` should have their enabled state updated based on context:

### Per-View Toggles (Active view context)

- `views:undo` - Enabled when active view has undo available
- `views:redo` - Enabled when active view has redo available
- `views:cut` - Enabled when active view has selection
- `views:copy` - Enabled when active view has selection
- `views:paste` - Enabled when active view can paste
- `views:delete` - Enabled when active view has selection
- `views:select_all` - Enabled when active view supports editing

### Per-File Toggles (Notepad)

- `notepad:save_file` - Enabled when current file is modified
- `notepad:save_file_as` - Enabled when current file exists

### Per-Window Toggles (Notepad)

- `notepad:save_all_in_window` - Enabled when window has any modified files

### Per-Workspace Toggles (Notepad)

- `notepad:save_all` - Enabled when workspace has any modified files

### Per-Archive Toggles (Notebook)

- `notebook:save_archive` - Enabled when archive is modified

### Tab Management Toggles

- `poly:close_tab` - Enabled when window has any tabs
- `poly:close_all_tabs_in_window` - Enabled when window has any tabs

### Future Toggle States

- `views:previous_tab` - Enabled when window has multiple tabs
- `views:next_tab` - Enabled when window has multiple tabs
- `windows:previous_window` - Enabled when workspace has multiple visible windows
- `windows:next_window` - Enabled when workspace has multiple visible windows
- `workspace:close_all_tabs` - Enabled when workspace has any tabs
- `workspace:close_all_windows` - Enabled when workspace has multiple windows

---

## Implementation Notes

### Command Parameters

Most commands accept:

```cpp
Command {
    QVariantMap params,  // Command-specific parameters
    Window* context      // Window context (can be nullptr)
}
```

Common parameter keys:

- `"index"` - Tab index (-1 for current)
- `"model"` - IFileModel pointer
- `"key"` - Settings key
- `"value"` - Settings value
- `"path"` - File path string

### Close Behavior Differences

**Notepad Close Tab:**

1. Check if model is modified
2. Check if model has multiple views across windows
3. If modified AND last view → prompt for save
4. Execute close

**Notebook Close Tab:**

1. Close view (temp file persists)
2. Archive tracks cumulative modifications
3. Prompt only when closing archive/workspace

### Save Behavior Differences

**Notepad:**

- File-granular: Each file saved independently to OS filesystem
- Models track modification state
- Multiple save variants for different scopes

**Notebook:**

- Archive-granular: Entire archive saved as one unit
- Archive modification state (cumulative of all file changes)
- Single save operation serializes all temp files to archive
