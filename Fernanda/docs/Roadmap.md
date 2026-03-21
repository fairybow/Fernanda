# Roadmap

## Current

- [x] CRITICAL BUG: Occasionally, saving Paradise Lost, I'll see a save fail prompt. It happens without the beforeWriteHook_ but not often. The hook doesn't seem to affect the frequency. Problem might be in Io::write and with QSaveFile? Here's the debug output that shows up related to this: `239 | 2026-03-20 | 22:11:19.360 | Failed to commit file at C:/.../Documents/Fernanda/Paradise Lost.txt (Error: Access is denied.)!` (see FileService writeModelToDisk_ for the fix)

- [ ] Possible to make Workspace-modal dialogs (for something like close all windows or tabs)?
- [ ] ^ If so, do a modality overhaul, checking dialogs and call sites for what window or workspace to pass (or set app modal, like for quit)
- [x] Comprehensive plan doc for backups/autosave
- [x] Backup folder in AppDirs
- [ ] Update docs (changelog, I think) re uninstalling (why .fernanda isn't deleted - contains backups)

- [x] Backup namespace
- [x] ^ Ability to clean up oldest backup past set limit
- [x] Hook spot in Fnx::compress
- [x] Hook spot in FilService (for Notepad to use)?
- [x] Notebook implementation
- [x] Notepad implementation
- [ ] Backups prune cap setting

- [ ] Chip/label on window to show it's in debug mode?

- [ ] Coco CMake?
- [x] Replace bit7z with miniz
- [x] Remove 7-Zip type from MagicBytes
- [x] Check for other bit7z/7-Zip references ("7-Zip" to ZIP or `.zip`)
- [x] BUG: Notepad TreeView allows moving files, which means FileMeta paths can become stale
- [x] BUG: Pulling tab off the tab bar after dragging it to a new position causes the dragged tab to be the other tab that was originally in that position!
- [x] BUG: Open several tabs and edit them. Then drag them all to new windows. Now, right click taskbar icon and close all windows. We'll see several pop-ups, one in each window with something unsaved. Hit "don't save" and the window should close with the tab. However, the window doesn't close (nor does the file). Then you can then close the window (with unsaved tab) and no pop-up shows to save, the window just closes (so either the file was closed without closing the tab the first time, or we aren't closing the file the second time maybe)?
- [x] BUG: ^ Actually, this happens when you open two windows (no tabs) and add a tab to each (no dragging) then right click taskbar icon > close all. Hitting "don't save" on one of those windows doesn't close that window or the tab in it. UNLESS you hit "don't save" on the top window first. Then everything seems to work as normal. Sounds like we're failing somewhere in WindowService with z-ordering. However, we can still edit the file (so the file model isn't being deleted, I guess), and closing the tab itself seems to prompt to save correctly (but not closing the window).
- [x] Update Readme build section
- [x] Clean up after file-types branch work (remove Plan document, update references)
- [x] Update documentation: FileModelsAndViews, FileHandling, Notebooks, Architecture
- [x] Merge file-types branch
- [ ] Have two "supported files" filters in addition to the "all files" (one for Notebook and one for Notepad which will also contain .fnx)
- [ ] These filters should be dynamically created in a Namespace that pulls from Tr for type names
- [ ] These filters should be set to list multiple extensions where applicable (meaning FileTypes needs an "all extensions" function to return canonical + aliases (JPG is the only one right now, when that's supported)
- [ ] Save As options could show a second (or third) filter (in addition to all files and/or all supported files) for the canonical type
- [ ] Need a function to return all extensions from the map (canonical + aliases)
- [x] Pull theme extensions from FileTypes
- [x] Will have to figure out how to reconcile the types ctor arg for models/metas with image types, as we'll only have one (ideally) ImageFileModel/View but several supported types for them
- [ ] Maybe clean-up MagicBytes + FileTypes enums, make sure they're consistent (or, better yet, maybe they share an enum from a new header that holds an enum val for every type Fernanda recognizes for any reason)
- [x] Add a rolling last used dir variable to Workspace (alongside startDir) that subclasses can use for their open/import dialogs
- [ ] Add a button to the open/import dialogs to navigate back to home (startDir)
- [x] Transition to CMake
- [ ] Ini key to defaults map
- [ ] Tab context menu (would have duplicate, save, save as, etc)
- [x] Uniform path display in save and reload prompts

- [ ] For Multi-file save prompt, can organize by directory, so we aren't repeating path for every file (just show parent as collapsible list and each individual name with check box (could also allow the list to be checked))

## File Types (Remaining)

From the file-types branch. See FileHandling.md for design details.

- [ ] FNX filter cleanup: Open Notebook, Save As Notebook, and Import dialogs still need `.fnx` filters. Consider a Filters header/namespace pulling translatable names from Tr and extensions from `Fnx::Io::EXT` / `FileTypes`.
- [ ] Tree view icons by file type: file-type-appropriate icons with a generic fallback for unrecognized types. `FnxModel::data()` can read `Fnx::Xml::ext(element)` and map through `FileTypes::fromPath()` for the `Qt::DecorationRole` case.
- [ ] Model/view re-evaluation on rename: when a file's extension changes, the system should swap to the appropriate model/view pair. Only affects Tier 2 types (special text), since Tier 1 (magic bytes) types are identified by content. Applies to both Notepad (filesystem rename) and Notebook (tree view rename).
- [ ] Consider NoOp for large unsupported binary files (e.g., images) where opening as text would be wasteful. Later-problem.
- [ ] FNX files within FNX archives: deliberately deferred. Nested archive lifecycle management conflicts with current architecture. See FileHandling.md for rationale.

## Follow-up Tags

Tags for working code that is a draft and/or needs more scrutiny/cleaning:

- TODO TD (tab dragging)
- TODO TVT (tree view toggle)
- TODO KFS (key filters settings)
- TODO ES (editor settings)
- TODO GH (grabbable highlight)
- TODO PD (prime doc)
- TODO FT (file types)
- TODO XP (cross platform)
- TODO STYLE
- TODO BA (backup/autosave)

## Features

- [ ] Autosave
- [ ] Spellcheck
- [ ] Find and replace
- [ ] Save backups
- [ ] Sessions for Notepad and Notebooks (Notepad sessions saved in User Data, Notebook in Archive Root)
- [ ] Pinned tabs (will utilize sessions)
- [ ] Tab groups (will utilize sessions, and maybe a better tab bar)
- [ ] Expanded/collapsed item restoration between sessions (will utilize sessions)
- [ ] Remembering dock position between sessions (using `QMainWindow::saveState()` / `restoreState()`)
- [ ] Status bar tools (AOT, Timer, Screen)
- [ ] Multiple TabWidgets per window for side-by-side
- [ ] Checkable export and compile feature with Dom tree
- [ ] Line spacing options, if possible
- [ ] Hide menu bar (key to toggle)
- [ ] Notebook LRU cache for models, if needed

## Refinement

- [ ] Window Themes (theming infrastructure is in place, but window theming proving difficult (especially with tab bar, buttons, and collapsible widget header (see: window QSS template)))
- [x] BUG: trash view: Just clicking the splitter handle will size trash view's closed state up to the splitter handle position. Additionally, trash view cannot be shrunk below the minimum we set
- [ ] Save backups (with auto-cleaning) and backup folder
- [ ] Backup folders for Notebook (FNX files only, not individual files) and Notepad saves, with auto clean up after n-files
- [ ] Ensure we are using terms around checkable items correctly in code (the term "toggle" is being used in a few different ways; checkable actions should be called checkables or similar)
- [ ] Make sure the distinction/usage between prompt, dialog, and box is clear
- [ ] Refactor save code in Notepad and Notebook
- [ ] Refactor common context menu items / TreeView hookup between Notepad and Notebook
- [ ] Notepad context menu (with rename, remove, collapse, expand, and anything else Notebook has that could apply)
- [ ] Renaming an open Notebook file in Notepad's TreeView: the Notebook works from its temp directory, so it is not broken immediately. The next save creates a file at the old path. No data loss, just a confusing orphaned file. Fix later.
- [x] Add note to AbstractFileView explaining why it needs two-step initialization (or remove it if unnecessary)
- [ ] Prevent clicking out of PTE context menu from affecting cursor on that first click
- [x] Custom context menu for AbstractFileView, implement for editors (replacing Qt editor context menu)
- [ ] Dual column layouts for some settings panels (instead of vertical stacks of checkboxes)
- [ ] Add defaults button to settings (should probably not write the defaults, since cascading should work for Notebook inheriting Notepad values)
- [ ] Multi-file save prompt file name clicks raising relevant window/file view
- [ ] Trim extra spaces after paragraphs on save (editor or workspace feature, not key filter)
- [ ] Trim extra space on newline/return (editor feature, not key filter)
- [ ] TreeView dock functionality refinement
- [ ] TreeView (NB): collapsed items should expand on hover while dragging
- [ ] TreeView (NB): items should expand when items are dragged into them
- [ ] TreeView (NB): expanded/collapsed states (probably a session thing)
- [ ] Trigger rename for new folders/files, but not import (maybe)
- [ ] Trash count
- [ ] MAYBE: Ensure "modules" are reactionary (ColorBars should probably stay a module, but right now it is called directly by Workspace)
- [ ] Remove tab size constraints in favor of QSS
- [ ] For tab drags, tab bar doesn't extend past the add tab button, so dropping there opens in a new window. May or may not be desirable.
- [ ] Ensure menu toggles update appropriately when tab dragging is implemented
- [ ] May want to remove commands for NxMenuModule to Workspace (can use signals). Would still need them for lateral NxMenuModule to other Service (like undo, redo, etc).
- [ ] Refactor common Notebook/Notepad opening code in Application
- [ ] Settings dialogs can have a section for App.ini (or similar), shared (like for startDir, when configurable)
- [ ] Settings files: Settings.ini inside Notebooks. Notepad file name may need to be different if we have application settings.
- [ ] Menu: prev/next tab, window, and workspace
- [ ] Another SoC audit, plus general audit, plus specifically Notepad/Notebook save and close code + Notebook trash code
- [ ] WidgetUtil or similar (central place to set all painters)
- [ ] Install preloaded fonts to system in help menu?
- [ ] Open data folder options in menus?
- [ ] temp AppDir could be an App TempDir

## Translations

Translations not worth it right now. Keep TR maybe. But remove all TR files but Spanish. Runtime translation is not going to happen without UI files. Would need to require restart and change TranslationDialog behavior, or just leave it alone for now.

All custom widgets would need to filter for change event and re-run Tr function calls:

```
void changeEvent(QEvent* event) override
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi_(); // re-apply all Tr:: calls
    }
    QWidget::changeEvent(event);
}
```

Might be simple but tedious. Later.

## Polish

- [ ] Shorter size abbreviations in Notepad TreeViews
- [ ] Tab bar tabs should not resize while user's mouse is over it (to enable rapid tab closing)
- [ ] .fnx Notebook icon
- [ ] Icon instead of trash text?
- [ ] Maybe stuff some of the closures in sub menus
- [ ] MenuBar highlight on hover, modeled after TabWidgetButton highlight (radius, color, etc.)
- [ ] Dock widget button/header styling

## Documentation

- [ ] Number and order for core docs
- [ ] Core docs should explain the program from start to finish (i.e., Openings.md, Modifications.md, Saves.md, Closures.md, etc.)
- [ ] Save prompts doc (preferred extension, how start paths are chosen/created)
- [ ] Saves of all types. Outline first everything that needs to be covered
- [x] How files get titles doc?
- [x] AbstractFileModel and View usage and purpose
- [ ] Explain debug and macros

## Coco

- [ ] Path dir iterator
- [ ] Path separator normalization: since this could be something maybe not always wanted, it could be set by a static "global" setter (like `Path::normalize(Posix)` in main) and relevant Path ops check against a static bool in source (or atomic)

## Logging and Debug

- [ ] Output buffered log to file (when available) if message is fatal (perhaps for critical and warn) before crashing
- [ ] Active window logging
- [ ] Active file view/model logging
- [ ] Debug/Utility function that shows a popup for messages (non-fatal)
- [ ] Red color bar for window on errors
- [ ] QSet printing (for WINDOWS_SET command result): needs type info storage or debug printer callback
- [ ] Log to file (commented-out method is too slow)
- [ ] Callback for debug printer that can be added while registering handler?
- [ ] Probably want a diagnostic/debug window that shows all files saved?

---

# Completed

## MVP/Misc

- [x] FNX (extract, compress, model, model manipulation, saving)
- [x] Menu item state toggling
- [x] Fully functioning Notepad (complete file/edit menus, open files, save them)
- [x] Fully functioning Notebooks (complete file/edit menus, open archive, make files, save archive, element removal)
- [x] Tab dragging (tab to window, tab to new window)
- [x] Basic key filters
- [x] Basic editor settings
- [x] Installer
- [x] Opening args
- [x] (Hopefully fixed) Bug: ColorBar sticking around - need an automatic shut off if still visible
- [x] View menu > toggle tree view (both Workspaces) > INI default (notepad off / notebook on) > write settings value either for an individual window toggle, switching current window, or both
- [x] Key filter settings toggles
- [x] Editor settings toggles
- [x] Grabbable selection
- [x] Moving preloaded fonts to top of selection box
- [x] Need a "dirty" mark on Notebook files in TreeView
- [x] Remove tab drop to new window positioning calculations and just start fresh!
- [x] Opening files via TreeView in both Workspaces
- [x] NewTab behavior for both Workspaces
- [x] Opening files via Menu in Notepad (Notebook menu won't open, just import)
- [x] Notebook Import
- [x] Granular FnxModel DOM updates
- [x] Renaming files in tree view updates the tab text
- [x] Moving/reorganizing Notebook files in TreeView
- [x] Replace Coco/TextIo with project version
- [x] Openings.md to explain New Tab procedures (and Notebook tab titles, too) and perhaps new windows and app open
- [x] Mononoki & OpenDyslexic
- [x] Settings dialog should have workspace name
- [x] Might be nice to have selection option for modified files in Notebook save, to exclude some changes from the archive save; would need to consult with FileService instead of ViewService and get all modified models (which, until/if LRU cache, remain open)
- [x] Change Notepad TreeView column (for filename) default width (too short)
- [x] Notebook Trash
- [x] Make cursor restart blink when clicking? Right now it looks like a delay or something
- [x] Allow double clicking to highlight white space (2 or more)
- [x] Update check
- [x] Align tab display text to not have leftmost text go past left side
- [x] Notebook (and Notepad, kinda) TreeView items indent too far
- [x] Choose most appropriate columns for Notepad TreeView
- [x] Remove unneeded menu items
- [x] Add window or view menu (or special Notebook menu that has import/export/open notepad/and anything else unique to NB)
- [x] Open notepad to window or view
- [x] Also need to have trash appear at the bottom
- [x] That might ensure, then, that the TreeView is stretched all the way and we can click anywhere not on an index to unselect the selected index
- [x] Removing unified New Tab command
- [x] Replacing New Tab with New Tab in Notepad, New File and New Folder in Notebook
- [x] Ensuring Notepad's ViewService uses New Tab for plus button, while NB's uses New File
- [x] Menu changes to accomodate, which includes an extra inserter
- [x] Ensuring all NB context menu items are added only when index is valid for them
- [x] TreeView toggling/redocking
- [x] Menu action toggling based on current view/model, window, workspace states
- [x] Save prompts
- [x] Saves
- [x] Working dir renames
- [x] New Notebook dialog
- [x] FnxModel + TreeView element removal
- [x] Trash view
- [x] Better arrow icon
- [x] Drag and drop from main to trash and back
- [x] Remove action sending to trash
- [x] Restore from trash
- [x] File opening and other main-like connections for trash view
- [x] Delete one item in trash (prompt, then delete/close model, file in working dir, and all views)
- [x] Empty trash (prompt, then delete/close all models/views/files from working dir)
- [x] No tree view for Notepad by default (but option to enable)?
- [x] Try Inno? (Follow-up: it's good!)
- [x] Ensure installer has: batchfile automated, optional shortcut, uses windeployqt6, also copies a shortcut to repo, copies inside a data folder with shortcut to exe inside at top level, and installer dir output is separated by platform somehow
- [x] Handle opening args!
- [x] Toggle logging based on VERSION_DEBUG in Version.h

## File Types

- [x] AbstractFileModel rework (pure virtual data/setData, virtual supportsModification)
- [x] FileTypes header (central type registry, canonical extensions)
- [x] Two-tier resolution in FileService (magic bytes first, extension second, plaintext fallthrough)
- [x] Remove non-FNX file dialog filters
- [x] Centralize model extensions (FileMeta::preferredExt)
- [x] Generalize FNX import (any file type, preserve source extension)
- [x] Generalize FNX new file creation (addNewFile takes FileTypes::Kind)
- [x] Extension attribute decision (kept in manifest, populated from reality)
- [x] Notepad rename from TreeView
- [x] Notebook export file
- [x] Settings converter pipeline (TieredSettings key-based converters for INI readability)
- [x] Notepad Save As custom extension fix
- [x] Pretty string for tab tooltip path

## Modifications

- [x] Window titles and flag (TODO NBM)
- [x] Need a method to modify elements via Notebook. Probably by UUID, which should be gotten from FileInfo and mapped maybe - how many problems would this cause? Maybe just query FnxModel for it somehow? Possible?
- [x] Marking Notebook as modified (TODO NBM)
- [x] Fnx file elements, add or remove edited attribute when model modification changes (TODO NBM)
- [x] FnxModel storing original DOM string + modified check method (TODO NBM)

## Closures

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
- [x] As part of Window/WindowService cleanup, ensure we still need custom Window::destroyed signal (`connect(view, &QObject::destroyed, this, [this, view] { /*clear view from a list*/ })` works fine)
- [x] Decide if Acceptor can be generalized AFTER. Don't get clever early!

- [x] SavePrompt: Save prompt should take either one Path or a list, and open the correct prompt type for either (and if the list has one item, open the single prompt there, too)
- [x] SavePrompt: Notepad save prompts
- [x] SavePrompt: Notebook save prompts

## Saves

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
- [x] Saves: Bug: (EASY - missing return statement for single-item delegation in SaveFailMessageBox::exec for string list) Unknown repro: had SaveFailMessageBox showing the successful files as a test, and when I added a new tab to a Notebook and then used Save As, I received two SaveFailMessageBox prompts for some reason? Should have only been one. I think they were for the same file, but not sure. Seems concerning!
- [x] Saves: MultiSave struct: add success count and aborted bool, allowing us to show no color bar if only aborted, green color bar if no fails but any successes before aborted, and red if any fails before aborted
- [x] Saves: MultiSave struct: General preference for color bar: failures take priority (any fails, show red); no saving means no color bar (so canceling a Save As and aborting early or on single file); if no failures and any success, show green
- [x] Saves: Change to normal order for file views collected
- [x] Saves: Consolidate Notepad Save As dialog occurences into one function
- [x] Saves: Preferred extension for off-disk files

## New Notebooks

- [x] New Notebook: Naming dialog (no path chosen)
- [x] New Notebook: Create new Notebook with the chosen name (no archive on disk, just working dir, will be modified)
- [x] New Notebook: On last window closure, app quit, or save/save as, prompt Save As with a base dir / Chosen name + .fnx
- [x] New Notebook: This base dir could replace Notepad's current working dir, would go in Workspace and be used by both Workspace types, settable by settings later (only problem is which settings? We have notepad and individual Notebook INIs...do we want/need an application-wide settings? How should we display that in the settings dialog for each Workspace?)
- [x] New Notebook: Isolate Save As logic from Notebook Save As (if trigger is closure/quit, we don't need to change fnxPath_, switch working dir, rebase model paths, change settings, or any of that stuff at the end of Notebook Save As handler; we also technically don't need to reset DOM snapshot or mark unmodified at the end of saveArchive_
- [x] New Notebook: May need to "unfactor" saveArchive_ to ensure we only do what's needed
- [x] New Notebook: For closure/quit, archive will be created and saved. If successful, we close the Notebook as normal (I think??? Am I missing anything?)
- [x] New Notebook: For Save / Save As (the former will trigger the latter anyway), the new Notebook will be saved like in the existing Save As handler. However, we'd only need to change the working directory if the path stem changed? This is a good argument for either just using a UUID or random string as the name (or simply keeping whatever name the Notebook had when it was opened/created, even if it's inconsistent with current name)
- [x] Need starting paths for Open Notebook, New Notebook (maybe). It's possible we may want Notepad to use an application wide base path (maybe set in Workspace and all Workspaces can access it) and settings can adjust it?

## Coco

- [x] Ability to log Coco::Bool
- [x] Remove any macros that take slots/lambdas, since a comma in the capture breaks them!
- [x] Redo path, potentially reintegrate PathUtil with Path (or Io umbrella file)
- [x] Figure out Path string caching
- [x] Ensure Path's shared data works
- [x] Path::isFolder to isDir
- [x] Path::copy (or Coco::copy)?
- [x] Move mkdir top level (Coco::mkdir) or Path?
- [x] Remove always inline macro!