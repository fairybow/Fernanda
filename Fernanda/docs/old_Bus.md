# Bus

TODO: Update tab/window closure items! Or remove and redirect to Closures.md

TODO: Could use a namespace/prefix to distinguish "public" commands (i.e., things used in menus) vs "internal" ones (like windows:set).

Commands are dynamically registered and can vary per workspace type. Events are statically defined signals for type safety and Qt integration. This asymmetry is intentional: commands are vocabulary, events are grammar.

## Commands

Commands are formatted as `scope:action`.

> [!NOTE]
> \* = Used by menus

### `application`

- `about_dialog`*: Displays an application-modal About dialog with version info and links.

### `workspace`

- `new_notebook`*: Opens a file dialog to create a new Notebook archive (`.fnx`), then opens it as a new Notebook workspace.
- `open_notebook`*: Opens a file dialog to select an existing Notebook archive (`.fnx`), then opens it as a new Notebook workspace.

### `poly` (removed)

Poly commands are registered per Workspace type (Notepad or Notebook) but called from Services or Modules that are Workspace agnostic.

- `new_tab`*: Creates a new tab with workspace-specific behavior.
    - **Notepad**: Opens a new tab (a new view on a new, off-disk file model).
    - **Notebook**: Adds a new empty file to the archive's temporary extraction folder, adds it to the Model.xml file, refreshes the tree view, and opens a new tab (a new view on a new on-disk file model for the new file).
- `close_tab`*: Closes the current tab with workspace-specific save handling.
    - **Notepad**: Checks if the model has views in other windows. If yes, just closes this view. If this is the last view on the model and the model is modified, prompts to save. On save/discard, closes the view and model.
    - **Notebook**: Closes the view without prompting. Model remains open to persist changes and undo/redo stacks.
- `close_all_tabs_in_window`*: Closes all tabs in the current window with workspace-specific save handling.
    - **Notepad**: Iterates backward through tabs. Builds a list of unique modified models that only have views in this window (skips models with views in other windows). If the list is not empty, shows multi-file save prompt with checkboxes. User can save selected files, discard all, or cancel. If cancel, aborts. Otherwise, saves chosen files, then closes all views and models.
    - **Notebook**: Simply closes all views without prompting.
- `ws_tree_view_model`: Returns the Workspace's file model (OS-based for Notepad and archive-based for Notebooks).
- `ws_tree_view_root_index`: Returns the Workspace's current TreeView root index.

### `notepad`

- `open_file`*: Opens a file dialog to select and open file(s), creating a new tab for each.
- `save_file`*: Saves current file to its path (or opens save dialog if not on disk).
- `save_file_as`*: Opens a save dialog to save current file with a new path.
- `save_all_in_window`*: Saves all modified files in the current window (prompting for paths as needed).
- `save_all`*: Saves all modified files across all Notepad windows (prompting for paths as needed).

### `notebook`

- `import_file`*: Opens a file dialog to copy selected file(s) into the archive's temp directory, marking archive as modified.
- `open_notepad`*: Activates the existing Notepad window or creates a new one if none exists.
- `save_archive`*: Saves the archive's temp directory contents back to the `.fnx` file.
- `save_archive_as`*: Opens a save dialog to save archive with a new path.
- `export_file`*: Opens a save dialog to copy the currently active file from the archive to the OS filesystem.

### `windows`

TODO: Verify `active` behavior for min/max, etc

Window service command handlers should be responsible for showing the window. When you call NEW_WINDOW, you expect to see a new window without having to show it yourself. Later, when we must open multiple windows for sessions, if we want to bubble them, we may have a different command that creates N windows at specified positions, and bubble-shows them itself.

- `new`*: Creates and shows a new window in the current workspace.
- `set`: Returns a `QSet` of all Workspace windows.
- `list`: Returns a `QList` of windows from top to bottom

### `views`

- `undo`*: Undoes the last edit operation in the active file view.
- `redo`*: Redoes the last undone edit operation in the active file view.
- `cut`*: Cuts the current selection to the clipboard from the active file view.
- `copy`*: Copies the current selection to the clipboard from the active file view.
- `paste`*: Pastes clipboard content into the active file view.
- `delete`*: Deletes the current selection in the active file view.
- `select_all`*: Selects all content in the active file view.

### `settings`

- `dialog`*: Opens a non-modal Settings dialog, or raises/activates it if already open.

### `color_bars`

- `run`: Run the context window's color bar.
- `run_all`: Run all workspace color bars.

### `file_models`

- `open_path`: Opens a file model for the file at the given path and readies it for a view. If the file model already exists, it is re-readied for an additional view. TODO: Add asterisk if in menu!

### `tree_views`

...

## Events (Signals)

Can be connected to and emitted by Services.

- `lastWindowClosed()`: Emitted when the last window in the workspace is closed
- `windowCreated(Window* context)`: Emitted when a new window is created
- `windowDestroyed(Window* context)`: Emitted when a window is destroyed
- `treeViewDoubleClicked(Window* context, const QModelIndex& index)`: Emitted when a tree view item is double-clicked
- `fileModelReadied(Window* context, AbstractFileModel* model)`: Emitted when a file model is readied for view creation
- `fileModelModificationChanged(AbstractFileModel* model, bool modified)`: Emitted when a file model's modification state changes
- `fileModelMetaChanged(AbstractFileModel* model)`: Emitted when a file model's metadata changes
- `activeFileViewChanged(Window* context, AbstractFileView* view)`: Emitted when a window's active file view changes
- `treeViewContextMenuRequested(Window* context, const QPoint& globalPos, const QModelIndex& index)`: Emitted when a TreeView is right-clicked
