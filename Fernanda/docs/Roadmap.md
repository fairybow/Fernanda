# Roadmap

## MVP

- [ ] FNX
- [ ] Functioning Notepad
- [ ] Functioning Notebooks
- [ ] Translations (FR, DE, ES, JA, ZH)

### Core Functionality (In Progress)

- [ ] Begin removing unneeded commands and calling public methods in Workspaces where appropriate. Determine what commands Workspace uses that are still used by other Services/Modules. Whatever isn't could be a public method.
- [ ] ^ Open file at path, new .txt, windows set (maybe) commands, etc
- [ ] ^ Virtual method for window close check
- [x] Opening files via TreeView in both Workspaces
- [x] NewTab behavior for both Workspaces
- [x] Opening files via Menu in Notepad (Notebook menu won't open, just import)
- [x] Notebook Import
- [x] Granular FnxModel DOM updates
- [x] Renaming files in tree view updates the tab text
- [x] Moving/reorganizing Notebook files in TreeView
- [ ] File closing
- [ ] File closing: Do Notebook "close all" and "close window" things first?
- [x] File closing: Add decrement logic for ViewService::viewsPerModel_ when a view is closed
- [x] File closing: MODEL_VIEW_COUNT command for Notepad to query when closing tab
- [x] File closing: Removing FileService's automatic model closure on last view close
- [ ] File closing: Likely, Notepad closes model on last view close
- [x] File closing: Likely, Notebook leaves models open
- [ ] File closing: Notebook uses LRU cache (may not need yet) to close some models (this would mean we'd need a persistence save again and a way to re-mark as modified on re-open?)
- [x] Notebook: Close all tabs in window
- [ ] Notepad: Close all tabs in window
- [ ] Update tab/window closure info in docs (Bus.md and Menus.md)
- [ ] Deleting Notebook on last window closure
- [ ] Quit procedures
- [ ] Notepad save prompts
- [ ] Marking Notebook as modified
- [ ] Notebook save prompts
- [ ] Notepad saving
- [ ] Notebook saving
- [ ] Application quit
- [ ] Notebook Trash (removing items, virtual op only since files will stay put but display in a "trash" section until trash is cleared)

### Documentation

- [x] Menus.md documentation (Ongoing)
- [x] Bus.md documentation (Ongoing)
- [ ] Opening Files doc (FileService usage, TreeView/Menu opening for both Workspaces, NewTab behavior)
- [ ] Saving Files doc (auto-save strategies, Notepad/Notebook saves, change persistence)
- [ ] UserData doc

### Architecture & Commands

- [ ] Implementing menus
- [ ] Redoing commands for menus as it is reimplemented
- [ ] Utility commands (windowsReversed, etc)
- [ ] Redo/organize Bus events
- [ ] Implement menu toggles
- [ ] Rethink Services/Modules distinction (proactive vs reactive framing)

### Stretch

- [ ] TreeView (NB): Collapsed items should expand on hover while dragging
- [ ] TreeView (NB): Items should expand when items are dragged into them
- [ ] TreeView (NB): Persist expanded states for items in Model.xml, so program remembers
- [ ] Trigger rename for new folders/files, but not import (maybe)?

## Code Quality & Refactoring

### Clean-up (High Priority)

- [ ] Check for places where std::forward would be appropriate (and where args can be forwarding ref)
- [ ] Where possible, make the command handler lambdas just wrap a private method (see ViewService) - much cleaner
- [ ] Clean up lambda captures (value capture may be volatile (re: C++ 20's `=` change), specify everything)
- [ ] Defensive QHash removals, when object is removed not just the window (see TreeViewModule)
- [ ] Search and resolve all TODO comments
- [ ] Remove "NOTE:" before notes, use only TODO or it's a note
- [ ] Use Bus windowDestroyed signal instead of connecting to window destroyed signals in onWindowCreated
- [ ] Uniform use of `nodiscard`
- [ ] Revise all class descriptions

### Command Handler Registration Clean-up

- [ ] Don't cast return values to QVar when registering handlers (it isn't needed)
- [ ] If handler isn't using a command, then make sure the lambda arg is empty (instead of `const Command&`)
- [ ] Check that if handler uses `cmd.context` that its nullptr-checked
- [ ] Ensure all command handler registrations use `cmd.param` instead of `to`

### General Clean-up

- [x] Replace Coco/TextIo with project version
- [ ] Standardize callback code for close acceptor and similar
- [ ] Find code that needs to be sectioned-off into a function for clarity
- [ ] Split to h/cpp where appropriate
- [ ] Move Internal namespaces to source files
- [ ] Ensure Internals are _ postfixed and possibly without namespace once in source
- [ ] Clean includes (Commands were in Constants briefly)
- [ ] Check license statements

### Coco Namespace Improvements

- [ ] Redo path, potentially reintegrate PathUtil with Path (or Io umbrella file)
- [ ] Figure out Path string caching
- [ ] Ensure Path's shared data works
- [ ] Path::isFolder to isDir
- [ ] Path::copy (or Coco::copy)?
- [ ] Move mkdir top level (Coco::mkdir) or Path?
- [ ] Move other non-Path stuff (search TODO)
- [ ] Path separator normalization

## Logging & Debug

### Logging Features

- [ ] Active window logging
- [ ] Active file view/model logging
- [ ] Debug/Utility function that shows a popup for messages (non-fatal)
- [ ] Red color bar for window on errors
- [ ] QSet<T> printing (for WINDOWS_SET command result) - needs type info storage or debug printer callback
- [ ] Ability to log Coco::Bool
- [ ] Log to file (commented-out method is too slow)

### Debug Infrastructure

- [ ] QDomDocument::ParseResult as model for save result or similar
- [ ] Callback for debug printer that can be added while registering handler

## Future

- [ ] Sessions for Notepad and Notebooks (Notepad sessions saved in User Data, Notebook in Archive Root)