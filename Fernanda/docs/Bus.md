# Bus

## Commands

Commands are formatted as `scope:action`.

Should we include registrar (i.e., `scope:registrar.action`)?

### `application`

- `quit`

### `workspace`

- `new_window`
- `new_notebook`
- `open_notebook`
- `close_window`
- `about_dialog`

### `poly`

Poly commands are registered per Workspace type (Notepad or Notebook) but called from Services or Modules that are Workspace agnostic.

- `new_tab`
- `close_tab`
- `close_all_tabs_in_window`

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

- `undo`
- `redo`
- `cut`
- `copy`
- `paste`
- `delete`
- `select_all`

### `settings`

- `dialog`

## Events

...
