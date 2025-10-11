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
Open File...                Ctrl+E          notepad:open_file
-------------------------------------------
Save                        Ctrl+S          notepad:save_file                [Toggle]
Save As...                  Ctrl+Alt+S      notepad:save_file_as             [Toggle]
Save All in Window                          notepad:save_all_in_window       [Toggle]
Save All                    Ctrl+Shift+S    notepad:save_all                 [Toggle]
```

## Notebook

### File

```
Import File...                              notebook:import_file
Open Notepad                                notebook:open_notepad
-------------------------------------------
Save                        Ctrl+S          notebook:save_archive            [Toggle]
Save As...                  Ctrl+Alt+S      notebook:save_archive_as
Export File...                              notebook:export_file
```

## Future Menu Items

- Previous tab (in current Window)
- Next tab (in current Window)
- Previous window (in current Workspace)
- Next window (in current Workspace)
- Close all tabs (in all Windows)
- Close all windows
- Close this file everywhere (closes all views on this model in all Windows)
