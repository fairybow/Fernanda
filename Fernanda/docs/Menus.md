# Menus

## Notes/Todo

Save will be fundamentally different for both Workspaces. Save, Save As, Save All, Save All In Window, etc, for Notepad. But for Notebook we only have Save, which will save the entire archive.

This means closing may also be different, as we may not need to save when closing an edited file (and we'll persist changes on the temp files until saved or discarded on exit).

Need to determine what's going to be a shared action (but may still have a differing implementation, like New Tab, which can be shared, but will be registered in Notepad and Notebook (and not FileService or Workspace), so will perform a different action for each Workspace (but same menu code)).

## Notepad

### File

- Open
- Save (Toggle)
- Save As (Toggle)
- Save All in Window (Toggle)
- Save All (Toggle)
- Close (Toggle)
- Close All in Window (Toggle)
- Close All (Toggle)
- Close Window
- Close All Windows

## Notebook

### File

- Save Notebook (Toggle)
- Save Notebook As
- Import
- Export
- Open Notepad

## Common

### File

- New Tab (different per Workspace)
- New Window
- New Notebook
- Open Notebook
- Close Tab (Toggle)
- Close All Tabs in Window (Toggle)
- Close All Tabs (Toggle)
- Close Window
- Close All Windows
- Quit

### Edit

- Undo (Toggle)
- Redo (Toggle)
- Cut (Toggle)
- Copy (Toggle)
- Paste (Toggle)
- Delete (Toggle)
- Select All (Toggle)

### View

- Previous Tab (Toggle)
- Next Tab (Toggle)
- Previous Window (Toggle)
- Next Window (Toggle)

### Settings

- Settings

### Help

- About
