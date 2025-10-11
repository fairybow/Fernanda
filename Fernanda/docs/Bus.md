# Bus

## Commands

Commands are formatted as `scope:action`.

### `application`

- `quit`: Terminates the entire application after attempting to close all Notebook and Notepad windows (prompting for saves as needed).

### `workspace`

- `new_window`: Creates and shows a new window in the current workspace.
- `new_notebook`: Opens a file dialog to create a new Notebook archive (.fnx), then opens it as a new Notebook workspace.
- `open_notebook`: Opens a file dialog to select an existing Notebook archive (.fnx), then opens it as a new Notebook workspace.
- `close_window`: Closes the specified window after running the workspace's close acceptor (which typically handles closing all tabs and save prompts).
- `about_dialog`: Displays an application-modal About dialog with version info and links.

### `poly`

Poly commands are registered per Workspace type (Notepad or Notebook) but called from Services or Modules that are Workspace agnostic.

- `new_tab`: Creates a new tab with workspace-specific behavior (new off-disk file in Notepad; new archive file in Notebook).
- `close_tab`: Closes the current tab, with workspace-specific save prompting (prompts in Notepad if modified and last view; no prompt in Notebook).
- `close_all_tabs_in_window`: Closes all tabs in the current window, with workspace-specific save handling (may prompt in Notepad; no prompt in Notebook unless last window).

### `notepad`

- `open_file`
- `save_file`
- `save_file_as`
- `save_all_in_window`
- `save_all`

### `notebook`

- `import_file`
- `open_notepad`
- `save_archive`
- `save_archive_as`
- `export_file`

### `views`

- `undo`: Undoes the last edit operation in the active file view.
- `redo`: Redoes the last undone edit operation in the active file view.
- `cut`: Cuts the current selection to the clipboard from the active file view.
- `copy`: Copies the current selection to the clipboard from the active file view.
- `paste`: Pastes clipboard content into the active file view.
- `delete`: Deletes the current selection in the active file view.
- `select_all`: Selects all content in the active file view.

### `settings`

- `dialog`: Opens a non-modal Settings dialog, or raises/activates it if already open.

## Events

...
