# Roadmap

## MVP

(Not necessarily in order)

- [x] Open Notepad command for menu
- [ ] FNX
- [ ] Functioning Notepad
- [ ] Functioning Notebooks
- [ ] Translations (FR, DE, ES, JA, ZH)
- [ ] Decide whether we need the other polys (new tab and tree model things)
- [ ] Save prompt should take either one Path or a list, and open the correct prompt type for either (and if the list has one item, open the single prompt there, too)
- [ ] Notepad save prompts
- [ ] Marking Notebook as modified
- [ ] Notebook save prompts
- [ ] Notepad saving
- [ ] Notebook saving
- [ ] Notebook Trash (removing items, virtual op only since files will stay put but display in a "trash" section until trash is cleared)

### Architecture & Commands

- [ ] Implementing menus
- [ ] Redoing commands for menus as it is reimplemented
- [ ] Utility commands (windowsReversed, etc)
- [ ] Redo/organize Bus events
- [ ] Implement menu toggles
- [ ] Rethink Services/Modules distinction (proactive vs reactive framing)

## Stretch

- [ ] TreeView (NB): Collapsed items should expand on hover while dragging
- [ ] TreeView (NB): Items should expand when items are dragged into them
- [ ] TreeView (NB): Persist expanded states for items in Model.xml, so program remembers
- [ ] Trigger rename for new folders/files, but not import (maybe)?

### Coco

- [ ] Redo path, potentially reintegrate PathUtil with Path (or Io umbrella file)
- [ ] Figure out Path string caching
- [ ] Ensure Path's shared data works
- [ ] Path::isFolder to isDir
- [ ] Path::copy (or Coco::copy)?
- [ ] Move mkdir top level (Coco::mkdir) or Path?
- [ ] Move other non-Path stuff (search TODO)
- [ ] Path separator normalization
- [ ] Remove any macros that take slots/lambdas, since a comma in the capture breaks them!

### Logging & Debug

- [ ] Active window logging
- [ ] Active file view/model logging
- [ ] Debug/Utility function that shows a popup for messages (non-fatal)
- [ ] Red color bar for window on errors
- [ ] QSet<T> printing (for WINDOWS_SET command result) - needs type info storage or debug printer callback
- [ ] Ability to log Coco::Bool
- [ ] Log to file (commented-out method is too slow)
- [ ] Callback for debug printer that can be added while registering handler?

### General

- [ ] Sessions for Notepad and Notebooks (Notepad sessions saved in User Data, Notebook in Archive Root)
- [ ] Notebook LRU cache for models, if needed
- [ ] Might be nice to have selection option for modified files in Notebook save, to exclude some changes from the archive save; would need to consult with FileService instead of ViewService and get all modified models (which, until/if LRU cache, remain open)

## Finished

- [x] Opening files via TreeView in both Workspaces
- [x] NewTab behavior for both Workspaces
- [x] Opening files via Menu in Notepad (Notebook menu won't open, just import)
- [x] Notebook Import
- [x] Granular FnxModel DOM updates
- [x] Renaming files in tree view updates the tab text
- [x] Moving/reorganizing Notebook files in TreeView
- [x] Replace Coco/TextIo with project version

### Closures

- [x] At each step of the way, decide what the "ideal" hook looks like, with proper encapsulation / separation of concerns in mind
- [x] Consider IService Coco::Bool or enum Accept/Reject for clarity
- [x] CLOSE_TAB in ViewService
- [x] Hook type in ViewService
- [x] Hook setter in ViewService
- [x] CLOSE_TAB virtual hook in Workspace
- [x] CLOSE_TAB hook implementation in Notepad
- [x] CLOSE_TAB hook implementation in Notebook
- [x] CLOSE_TAB_EVERYWHERE in ViewService
- [x] Hook type in ViewService
- [x] Hook setter in ViewService
- [x] CLOSE_TAB_EVERYWHERE virtual hook in Workspace
- [x] CLOSE_TAB_EVERYWHERE hook implementation in Notepad
- [x] CLOSE_TAB_EVERYWHERE hook implementation in Notebook
- [x] CLOSE_WINDOW_TABS in ViewService
- [x] Hook type in ViewService
- [x] Hook setter in ViewService
- [x] CLOSE_WINDOW_TABS virtual hook in Workspace
- [x] CLOSE_WINDOW_TABS hook implementation in Notepad
- [x] CLOSE_WINDOW_TABS hook implementation in Notebook
- [x] CLOSE_ALL_TABS in ViewService
- [x] Hook type in ViewService
- [x] Hook setter in ViewService
- [x] CLOSE_ALL_TABS virtual hook in Workspace
- [x] CLOSE_ALL_TABS hook implementation in Notepad
- [x] CLOSE_ALL_TABS hook implementation in Notebook
- [x] AT THIS POINT: Doublecheck ViewServices implementations and hooks
- [x] Windows close via close method
- [x] WindowService flag for individual window closes during a multi-window close
- [x] Hook type in WindowService
- [x] Hook setter in WindowService
- [x] CLOSE_WINDOW virtual hook in Workspace
- [x] CLOSE_WINDOW hook implementation in Notepad
- [x] CLOSE_WINDOW hook implementation in Notebook
- [x] CLOSE_ALL_WINDOWS in WindowService
- [x] Hook type in WindowService
- [x] Hook setter in WindowService
- [x] CLOSE_ALL_WINDOWS virtual hook in Workspace
- [x] CLOSE_ALL_WINDOWS hook implementation in Notepad
- [x] CLOSE_ALL_WINDOWS hook implementation in Notebook
- [x] Quit virtual in Workspace
- [x] Quit implementation in Notepad
- [x] Quit implementation in Notebook
- [x] App's quit routine (for each N in Notebooks, N->quit(); Notepad->quit(); App quits)
- [x] App's passive quit (when no windows are open)
- [x] Ensure system shutdown is handled with app's quit routine
- [x] As part of Window/WindowService cleanup, ensure we still need custom Window::destroyed signal (`connect(view, &QObject::destroyed, this, [&, view] { /*clear view from a list*/ })` works fine)
- [x] Decide if Acceptor can be generalized AFTER. Don't get clever early!
