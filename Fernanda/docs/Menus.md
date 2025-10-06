# Menus

TODO: Add description of each item.

## Common

### File

```
New tab                     Ctrl+D          poly:new_tab
New window                  Ctrl+W          workspace:new_window
-------------------------------------------
New notebook                                workspace:new_notebook
Open notebook                               workspace:open_notebook
```

```
[Save section is per subclass]
```

`Close window` will likely utilize `Close all tabs in window`.

```
Close tab                                   poly:close_tab                   [Toggle]
Close all tabs in window                    poly:close_all_tabs_in_window    [Toggle]
-------------------------------------------
Close window                                workspace:close_window
-------------------------------------------
Quit                        Ctrl+Q          application:quit
```

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
