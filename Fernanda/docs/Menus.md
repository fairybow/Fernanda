# Menus

TODO: Update tab/window closure items!

TODO: Make sure TR lines up

TODO: Description of menu implementation (no base class, two modules that utilize shared menu-builder methods and utilities and action structs that each have a member containing common actions)

## Common

### File

TODO: Will new/open notebook be app scope?

```
New tab                     Ctrl+D          poly:new_tab
New window                  Ctrl+W          windows:new
-------------------------------------------
New notebook                                workspace:new_notebook
Open notebook                               workspace:open_notebook
-------------------------------------------
[Save section is per subclass]
-------------------------------------------
Close tab                                   poly:close_tab                   [Toggle]
Close tab everywhere                        poly:close_tab_everywhere        [Toggle]
Close window tabs                           poly:close_window_tabs           [Toggle]
Close all tabs                              poly:close_all_tabs              [Toggle]
-------------------------------------------
Close window
Close all windows                           poly:close_all_windows
-------------------------------------------
Quit                        Ctrl+Q
```

**New tab (poly):**

- In Notepad: Opens a new tab (a new view on a new, off-disk file model).
- In Notebook: Adds a new empty file to the archive's temporary extraction folder, adds it to the Model.xml file, refreshes the tree view, and opens a new tab (a new view on a new on-disk file model for the aforementioned new file).

**New window:**

- Opens a new window.

**New notebook:**

- Opens a file dialog for Notebook creation, then opens the new Notebook.

**Open notebook:**

- Opens a file dialog for existing Notebook selection, then opens the selected Notebook.

For Close tab, Close tab everywhere, Close window tabs, Close all tabs, Close window, Close all windows, and Quit, see [Fernanda/docs/Closures.md](Closures.md).

### Edit

```
Undo                        Ctrl+Z          views:undo                       [Toggle]
Redo                        Ctrl+Y          views:redo                       [Toggle]
-------------------------------------------
Cut                         Ctrl+X          views:cut                        [Toggle]
Copy                        Ctrl+C          views:copy                       [Toggle]
Paste                       Ctrl+V          views:paste                      [Toggle]
Delete                      Del             views:delete                     [Toggle]
-------------------------------------------
Select all                  Ctrl+A          views:select_all                 [Toggle]
```

Toggle conditions here are based on the active file in the window.

### Settings

```
Settings                                    settings:dialog
```

### Help

```
About                                       application:about_dialog
```

## Notepad

### File

```
Open file...                Ctrl+E          notepad:open_file
-------------------------------------------
Save                        Ctrl+S          notepad:save_file                [Toggle]
Save as...                  Ctrl+Alt+S      notepad:save_file_as             [Toggle]
Save all in window                          notepad:save_all_in_window       [Toggle]
Save all                    Ctrl+Shift+S    notepad:save_all                 [Toggle]
```

**Open file...:**

- Opens a file dialog and opens a new tab for each selected file.

**Save (toggle):**

- If file is on disk, saves to its current path. If file is not on disk, opens a save dialog first (like Save As).
- Toggle condition: Current file is savable and modified.

**Save as... (toggle):**

- Opens a save dialog, saves file to selected path, and updates the file's path.
- Toggle condition: Current file is savable.

**Save all in window (toggle):**

- Saves all modified files in the current window. For any files not on disk, opens save dialogs.
- Toggle condition: Window has any modified files.

**Save all (toggle):**

- Saves all modified files across all Notepad windows. For any files not on disk, opens save dialogs.
- Toggle condition: Workspace has any modified files.

## Notebook

### File

```
Open notepad                                notebook:open_notepad
-------------------------------------------
Import file...                              notebook:import_file
-------------------------------------------
Save                        Ctrl+S          notebook:save_archive            [Toggle]
Save as...                  Ctrl+Alt+S      notebook:save_archive_as
Export file...                              notebook:export_file
```

**Import file...:**

- Opens a file dialog, copies selected file(s) to the archive's temp extraction directory, updates the model, and marks archive as modified.

**Open Notepad:**

- Activates the top-most existing Notepad window if present or creates a new one.

**Save (toggle):**

- Copies the archive's extraction directory to the archive.
- Toggle condition: Archive modification changes.

**Save As...:**

- Opens a save dialog, copies the archive's extraction directory to a new archive path, and updates the workspace's archive path.

**Export file...:**

- Opens a save dialog and copies the currently active file to the selected OS location.

## Future Menu Items

- Previous tab (in current Window)
- Next tab (in current Window)
- Previous window (in current Workspace)
- Next window (in current Workspace)
