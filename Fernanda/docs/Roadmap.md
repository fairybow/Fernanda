# Roadmap

## MVP

- [ ] FNX
- [ ] Functioning Notepad
- [ ] Functioning Notebooks
- [ ] Translations (FR, DE, ES, JA, ZH)

### Core Functionality (In Progress)

- [ ] Begin removing unneeded commands and calling public methods in Workspaces where appropriate. Determine what commands Workspace uses that are still used by other Services/Modules. Whatever isn't could be a public method.
- [ ] ^ Open file at path, new .txt, windows set (maybe) commands, etc
- [ ] ^ Open file is complicated because of the need for interception (need to reconcile this command, anyway, with the Notepad and Notebook open/import file commands)
- [x] Opening files via TreeView in both Workspaces
- [x] NewTab behavior for both Workspaces
- [x] Opening files via Menu in Notepad (Notebook menu won't open, just import)
- [x] Notebook Import
- [x] Granular FnxModel DOM updates
- [x] Renaming files in tree view updates the tab text
- [x] Moving/reorganizing Notebook files in TreeView

- [x] TODO KEY = CR (Closure Rework)
- [x] At each step of the way, decide what the "ideal" hook looks like, with proper encapsulation / separation of concerns in mind
- [ ] Consider IService Coco::Bool or enum Accept/Reject for clarity
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
- [ ] As part of Window/WindowService cleanup, ensure we still need custom Window::destroyed signal (`connect(view, &QObject::destroyed, this, [&, view] { /*clear view from a list*/ })` works fine)
- [ ] Decide if Acceptor can be generalized AFTER. Don't get clever early!
- [ ] Decide after whether we need the other polys (new tab and tree model things)
- [ ] Finally, find all functions rendered unused by these changes and remove them!

- [ ] Save prompt should take either one Path or a list, and open the correct prompt type for either (and if the list has one item, open the single prompt there, too)

- [ ] All commands should just call functions (like in ViewService), looks much cleaner, easier to follow
- [ ] Make sure services aren't calling their own commands, dum-dum
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
- [ ] Ensure setup_ methods are only called by ctor (not in an initialize function); they should have only ctor-friendly setup, too
- [ ] Ensure functions are well-named (actions taken on windows have the right preposition, for example (like `openFileIn(window)`))
- [ ] Standardize callback code for close acceptor and similar
- [ ] Find code that needs to be sectioned-off into a function for clarity
- [ ] Split to h/cpp where appropriate
- [ ] Move Internal namespaces to source files
- [ ] Ensure Internals are _ postfixed and possibly without namespace once in source
- [ ] Clean includes (Commands were in Constants briefly)
- [ ] Check license statements
- [ ] Would really like to see functions organized by category (e.g., public query methods, then public actions, or whatever)

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
- [ ] Notebook LRU cache for models, if needed
- [ ] Might be nice to have selection option for modified files in Notebook save, to exclude some changes from the archive save; would need to consult with FileService instead of ViewService and get all modified models (which, until/if LRU cache, remain open)

Find what needs automatic clean-up from member lists/hashes/sets and ensure we do so, e.g.:

```
modelViews_[model] << view;
connect(view, &QObject::destroyed, this, [&, view, model] {
    modelViews_[model].remove(view);
});
```
