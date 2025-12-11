# Roadmap

## Features

### Up Next (See Miscellaneous Todos)

- [x] Save prompts
- [ ] Saves
- [x] Warking dir renames
- [ ] New Notebook dialog
- [ ] Notebook export file
- [ ] FnxModel + TreeView element removal
- [ ] Backup folders for Notebook (FNX files only, not individual files) and Notepad saves, with auto clean up after n-files

### MVP

- [ ] FNX (extract, compress, model, model manipulation, saving)
- [ ] Menu item state toggling
- [ ] Fully functioning Notepad (complete file/edit menus, open files, save them)
- [ ] Fully functioning Notebooks (complete file/edit menus, open archive, make files, save archive, element removal)
- [ ] Tab dragging (tab to window, tab to new window)
- [ ] Basic key filters
- [ ] Translations (FR, DE, ES, JA, ZH)
- [ ] Basic editor settings

### Stretch

- [ ] Notebook Trash
- [ ] Spellcheck
- [ ] Find and replace
- [ ] Status bar tools (AOT, Timer, Screen)
- [ ] Styling/themes
- [ ] Sessions for Notepad and Notebooks (Notepad sessions saved in User Data, Notebook in Archive Root)
- [ ] Multi-file save prompt file name clicks raising relevant window/file view

### Docs

- [ ] Number and order for core docs
- [ ] Core docs should explain the program from start to finish (i.e., Openings.md, Modifications.md, Saves.md, Closures.md, etc.)
- [ ] Save prompts doc?
- [ ] How files get titles doc?
- [ ] AbstractFileModel and View usage and purpose

## Miscellaneous Todos

- [x] SavePrompt: Save prompt should take either one Path or a list, and open the correct prompt type for either (and if the list has one item, open the single prompt there, too)
- [x] SavePrompt: Notepad save prompts
- [x] SavePrompt: Notebook save prompts

(Search TODO SAVES)
- [x] Saves: FileService should be saving, not AbstractFileModel
- [x] Saves: AbstractFileModel needs some sort of content function returning QByteArray
- [x] Saves: TextIo can probably be more generalized and work on QByteArray
- [x] Saves: Notepad and Notebook would register their respective "Save" command handlers, since their saves are different and not just FileService::save
- [x] Saves: Notepad does just call FS::save
- [x] Saves: Notepad success on multi-save should just run green color bar; failure, though, should show red color bar and a pop-up of which specific files failed to save
- [x] Saves: Notebook performs two-tier save, first calling FS::save for all modified models and saving to working dir, then compressing and saving archive
- [x] Saves: (Notebook) Ensure edited attributes are cleared and written before compressing and replacing archive
- [x] Saves: (Notebook) Ensure DOM snapshot is replaced on save
- [x] Saves: Remove red color bar on Save As dialog abort
- [x] Saves: Decide how to handle red color bar on Notepad multiSave_
- [x] Saves: Bug: Can't see green color bar on window close, as expected, so remove them
- [x] Saves: Bug: Notepad isn't closing models sometimes (repro: have 1 NP window, open two files, edit both, close window, prompt save, uncheck one file, save the other, reopen notepad, open both files, the skipped one is still edited (model never died))
- [x] Saves: Bug(?): Notebook working dir/temp folder name doesn't change after Save As, and I'm not sure if that matters other than a user might expect a change if they ever need to access temp folders somehow
- [ ] Saves: Probably want a diagnostic/debug window that shows all files saved?
- [x] Saves: Bug: (EASY - missing return statement for single-item delegation in SaveFailMessageBox::exec for string list) Unknown repro: had SaveFailMessageBox showing the successful files as a test, and when I added a new tab to a Notebook and then used Save As, I received two SaveFailMessageBox prompts for some reason? Should have only been one. I think they were for the same file, but not sure. Seems concerning!
- [x] Saves: MultiSave struct: add success count and aborted bool, allowing us to show no color bar if only aborted, green color bar if no fails but any successes before aborted, and red if any fails before aborted
- [x] Saves: MultiSave struct: General preference for color bar: failures take priority (any fails, show red); no saving means no color bar (so canceling a Save As and aborting early or on single file); if no failures and any success, show green
- [x] Saves: Change to normal order for file views collected
- [ ] Saves: Consolidate Notepad Save As dialog occurences into one function
- [ ] Saves: Document! Outline first everything that needs to be covered

- [ ] Preferred extension for off-disk files
- [ ] Getting the preferred or current extension to set the selected filter (if it isn't automatically)?
- [ ] Document this also

### Coco

- [ ] Basically redo the whole thing!
- [ ] Remove always inline macro!
- [ ] Path dir iterator
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
- [x] Ability to log Coco::Bool
- [ ] Log to file (commented-out method is too slow)
- [ ] Callback for debug printer that can be added while registering handler?

### General

- [ ] Notebook LRU cache for models, if needed
- [ ] Might be nice to have selection option for modified files in Notebook save, to exclude some changes from the archive save; would need to consult with FileService instead of ViewService and get all modified models (which, until/if LRU cache, remain open)
- [ ] Trigger rename for new folders/files, but not import (maybe)?
- [ ] TreeView (NB): Collapsed items should expand on hover while dragging
- [ ] TreeView (NB): Items should expand when items are dragged into them
- [ ] TreeView (NB): Persist expanded states for items in Model.xml, so program remembers

### Finished

- [x] Opening files via TreeView in both Workspaces
- [x] NewTab behavior for both Workspaces
- [x] Opening files via Menu in Notepad (Notebook menu won't open, just import)
- [x] Notebook Import
- [x] Granular FnxModel DOM updates
- [x] Renaming files in tree view updates the tab text
- [x] Moving/reorganizing Notebook files in TreeView
- [x] Replace Coco/TextIo with project version
- [x] Openings.md to explain New Tab procedures (and Notebook tab titles, too) and perhaps new windows and app open

#### Modifications

- [x] Window titles and flag (TODO NBM)
- [x] Need a method to modify elements via Notebook. Probably by UUID, which should be gotten from FileInfo and mapped maybe - how many problems would this cause? Maybe just query FnxModel for it somehow? Possible?
- [x] Marking Notebook as modified (TODO NBM)
- [x] Fnx file elements, add or remove edited attribute when model modification changes (TODO NBM)
- [x] FnxModel storing original DOM string + modified check method (TODO NBM)

#### Closures

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
