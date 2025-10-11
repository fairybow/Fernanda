# Menus

TODO: Add description of each item.

TODO: Ensure we describe poly commands in command doc

## Common

### File

```
New tab                     Ctrl+D          poly:new_tab
New window                  Ctrl+W          workspace:new_window
-------------------------------------------
New notebook                                workspace:new_notebook
Open notebook                               workspace:open_notebook
-------------------------------------------
[Save section is per subclass]
-------------------------------------------
Close tab                                   poly:close_tab                   [Toggle]
Close all tabs in window                    poly:close_all_tabs_in_window    [Toggle]
-------------------------------------------
Close window                                workspace:close_window
-------------------------------------------
Quit                        Ctrl+Q          application:quit
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

**Close tab (poly, toggle):**

- In Notepad: Check if this model has views in other windows. If yes, just close this view. If this is the last view on the model: check if modified. If modified, prompt to save. On save/discard, close the view and emit `viewClosed`. FileService will clean up the model when view count reaches zero.
- In Notebook: Close the view without prompting. If this was the last view on the model, changes remain in the temp file and the archive stays marked as modified. FileService cleans up the model when view count reaches zero.
- Toggle condition: Window has any tabs.

**Close all tabs in window (poly, toggle):**

- In Notepad: Iterate backward through tabs. Build a list of unique modified models that only have views in this window (skip models with views in other windows). If the list is not empty, show multi-file save prompt with checkboxes. User chooses: save selected files, discard all, or cancel. If cancel, abort. Otherwise, save chosen files, then close all views and emit events for each.
- In Notebook: Simply close all views without prompting. Changes remain in temp files, archive stays marked as modified if applicable.
- Toggle condition: Window has any tabs.

**Close window:**

- Calls the window close acceptor, which delegates to "close all tabs in window" logic.
- In Notepad: This may prompt for saves (via close all tabs logic).
- In Notebook: If this is NOT the last window, close tabs without prompting. If this IS the last window, check if archive is modified. If modified, prompt to save the archive. On save/discard, close tabs and clean up temp directory. On cancel, abort window close.
- WindowService emits `windowDestroyed` on success. If this was the last window in the workspace, emits `lastWindowClosed`.

**Quit:**

- First, iterate through all Notebook workspaces. For each: check if modified, prompt to save archive if needed, close all windows. If any Notebook close is canceled, abort quit.
- Then, iterate through Notepad windows (reverse z-order). Call close window for each. If any window close is cancelled, abort quit.
- If all closes succeed, call `Application::quit()`.

**Notes:**

- `Close window` will likely utilize `Close all tabs in window`.
- Notebook's modified state is cumulative (tracked at workspace level) while Notepad's is granular (tracked per file model).

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
About                                       workspace:about_dialog
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
Import file...                              notebook:import_file
Open notepad                                notebook:open_notepad
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
- Close all tabs (in all Windows)
- Close all windows
- Close this file everywhere (closes all views on this model in all Windows)
