# Roadmap

Also, go through all docs and make sure they're up to date and follow doc style

## Follow-up Tags

Tags for working code that is a draft and/or needs more scrutiny/cleaning:

- TODO TD (tab dragging)
- TODO PD (prime doc)
- TODO FT (file types)
- TODO XP (cross platform)
- TODO STYLE
- TODO BA (backup/autosave)
- TODO MU (markups)
- TODO NF (new file options (e.g., new Markdown)
- TODO IV (image views)
- TODO TS (tab splits)
- TODO RN (program rename)

---

## Current

- [ ] Themed focus indicator color for plain text edit
- [ ] Extend line highlight across whole editor?
- [ ] ^ Also, a bug with line highlight - sometimes white vertical line appears in it
- [x] Dividing line between line number area and text
- [ ] Open blank tab on Notepad startup with no other args
- [ ] Icon chevron ("hearth") looks floppy at small scale
- [ ] Icon is also set a little high on taskbar for some reason

- [ ] Change ColorBar colors, add Warm (re-add beCute?)
- [x] Remove .hearthx from Notebook color chip
- [ ] Clarity around NBX vs hearthx vs Hearth Notebook. If the former is the spec and the latter two are the extension and Hearth-specific usage of NBX, then there's still incorrect usage in code (e.g., isNbxFile checks for the extension, which is not defined by NBX but by Hearth, so it should be isHearthxFile or similar)

Pain points while using:

- [x] Side-by-side tabs
- [x] New tab area container widget
- [x] ViewService refactor for multiple TabWidgets
- [x] VS resolves to model before allowing hook (See below)
- [x] Workspace hook refactor (Window + AFM, not Window + index)
- [x] moveToSplit_ does a raw removeTab + addTab rather than going through the hook system, since it isn't a close operation. The view stays alive and just moves to a different container. If the source split ends up empty after the move, splitEmpty will fire from TabSurface, and we can decide later whether to auto-collapse it or leave it
- [x] Tab dragging
- [x] Menu commands (split right/left; should move the active tab in active TabWidget to left or right, into existing TabWidget if present or creating a new one)
- [ ] Drop zone visual feedback
- [x] Valid cursor for passthrough drop path
- [x] For review: TabSurface.h; ViewService.h (TabSurface integration, split operations, hook signature changes, auto-collapse suppression, cleanup); Workspace.h (hook signature changes, context menu split actions, bus event for split count, fixed centralWidget casts); Workspace.cpp (File menu Split submenu); Bus.h (new splitCountChanged signal); Notepad.h (hook signatures: canCloseTab, canCloseTabEverywhere); TabWidget.h (new SplitSide enum, tabDraggedToSplitEdge signal, dragStarted/dragEnded signals, DropZone_ enum, dropZone_ helper); TabWidget.cpp (dragMoveEvent, dropEvent edge zone handling, startDrag_ drag lifecycle signals)
- [ ] Sessions (tab splits, open tabs, pinned tabs (future), window positions, TreeView sizes, Notebook expanded items)
- [x] Option to disallow multiple tabs onto same model
- [ ] Option to turn tabs off for Notebook!
- [ ] ^ Title bar to show what's open when tabs are off
- [ ] ^ Context menu for title bar to close split (if not last split) and duplicate open document, plus anything else relevant
- [ ] ^ Handle existing menu item toggling or visibility (what actions in File menu won't be applicable with no tabs)?
- [ ] ^ For Notebook, we can just close all tabs whenever, no saving needed, so toggling the feature on could probably just close all tabs?
- [ ] ^ we'd probably want this setting to override unique tabs (mandating unique tabs for the Notebook workspace)
- [x] Close all tabs option in tab context menu
- [ ] (Update all documentation)
- [ ] Investigate large (new file count) Notebook saves (maybe choked a little when saving Save the Cat! template)
- [x] Clean Workspace
- [x] Refactor ViewService
- [ ] Code clean-up: try to remove basically all onXHappened_ slots (because it is not descriptive - if only one thing is happening, just name it appropriately); only give it onXHappened_ name if it wraps multiple function calls
- [ ] Code clean-up: drop unneeded connection slot/lambda parameters, instead of [[maybe_unused]]
- [ ] Code clean-up: also drop param names on virtuals when they're self explanatory (e.g., not Window* window)
- [ ] Code clean-up (tweak, add to CodeStyle.md):

```
Within each access section, order members as:

Type aliases and nested types
Constructors
Destructor
Methods (ordered: static, virtual/overrides, regular, unless sectioned in a specific way)
Data members

This ordering applies uniformly to public, protected, and private sections. Readers encounter what the class is and does before its internal state.

POSSIBLY switch members and methods for private only
```

---

- [x] We're not accounting for scroll bars in resizing to fit in ImageFileView (so, starts Fit, looks fine, switch, then switch back, and we'll have a scroll bar sized gap)
- [x] Clean ImageFileView, ImageGraphicsView, ZoomControl, and PdfFileView headers
- [x] Isolate ImageFileView, ImageGraphicsView, ZoomControl, and PdfFileView repetitive code
- [x] ZoomState extraction (see above)
- [x] (No) GIFs may need loading mask
- [x] If so, extract common loading mask code (including maybe warmup mask) somehow (in WebEngineView and AbstractMarkupFileView)
- [x] ^ WebEngineView itself can have a first load / warm up static bool which consumers can check against to apply a warm-up mask to their entire widget (AMUFV and HTML view)
- [x] ZoomControl-owned (not StyleContext) plus/minus SVGs
- [x] (No) Maybe separate UI folder, then, and make StyleContextIcons or something
- [ ] Check all enums and don't first one = 0 if it isn't treated as a sentinel anywhere
- [x] Use using namespace Qt::StringLiterals throughout (not just where it can't leak)?
- [ ] Remove unnecessary includes for AbstractFileModel or AbstractFileView (kind of a bridge too far in terms of being explicit about types included)

- [ ] Sample stuff (a folder in Docs/Hearth with samples of each type - can be recreated from the Help menu)
- [ ] Use QIconEngine for StyleContext icons?
- [x] Probably add a warm-up mask to the web engine view itself, too (in addition to the AMUFV as a whole?)
- [x] HtmlFileView (This will not be editable, like other markups - this is for research, essentially, saved pages)
- [x] Combine ImageFileModel and HtmlFileModel into Blob/Raw/GenericFileModel (no functionality, just byte bag)
- [x] Figure out how to open correctly in FileService
- [x] Rename MarkupPreviewPage since it is used by HtmlView (not a preview)
- [x] Subclass web engine view and remove the context menu for now (has options we probably won't want used, e.g. view source)
- [x] HtmlFileView web engine view needs the same masking the markup views have for resize / init (probably add the the webengineview subclass?)
- [x] Animated Gifs
- [ ] NbxModel::data file/folder metadata tooltip
- [x] ControlField tooltip quicker popup time? Possible?
- [x] ^ OR, allow click to show tooltip, too (or both)
- [ ] DisplaySlider click to edit and set value (clamp)
- [x] Also debounce slider settings emissions to reduce the log output (actually won't reduce it, since we still call commands on every slide to propagate the visual changes)
- [ ] Notebook "save webpage" with URL entry (a la other binder-style writing apps)
- [ ] *maybe* Remove Formatters and std::format. We only ever use {} args, no special specifiers. We could make ToString very robust and just do our own string replacements
- [ ] ^ Add regular ptr to ToString; add toQString wrappers for QString::number, etc, to reduce constexpr chain in Debug
- [ ] ^ Also maybe move the format code to a "Format" header
- [x] (Nah) typedef int Index? Will I be laughed at?
- [ ] MAYBE use clang InsertBraces = true plus remove virtual on overrides
- [ ] SoC Audit!
- [ ] Audit FileService's file opening functions (newOnDisk vs newOffDisk; only plain text types are creatable; is naming appropriate for each?; does each methods usage in Notepad vs Notebook make sense?; are the methods themselves handling their responsibilities correctly?; does everything in each method belong specifically in that method?; etc)
- [ ] See Notebook::fileMenuOpenActions note (but I'm wary of giving Notepad folder-creation)
- [ ] Tab context menu: add close to right, left and all other
- [ ] The u prefix makes a char16_t literal (UTF-16). Qt's QString and QStringView are UTF-16 internally, so comparisons like reader.name() == u"p" avoid any implicit conversion - check for other similar occurrences and fix!
- [ ] Audit containers used to track objects (like hash maps of Window* to Object* (word counter, color chip, etc). Make sure they all clean up in a uniform way (and then perhaps extract that clean-up logic to namespace) (search // TODO: Tracking/clean-up helper) (add an `if (hash.removed) log("msg");` pattern; use: obj, QObject::destroyed, context, lambda w/ removal)
- [ ] Add installer options to open .txt, .fountain, .md
- [ ] Clean up Bus slots (onAction_ convention shouldn't mask what's happening in the function - we should use that when we have multiple other methods to call; e.g. xModified calls onXModified_ which then itself calls setXRelatedThing1_, setOtherThing_, etc...)
- [ ] Clang rule to always use brackets on if/while statements, even single line, UNLESS that single line would not wrap (e.g. this is fine: `if (0) ++i;`)
- [ ] setCursor(Qt::PointingHandCursor) for other buttons
- [ ] Highlight/Q_PROPERTY for all buttons (before tackling window style/themes)
- [ ] drop `auto` ???
- [ ] AbsMarkupFileView mode bar needs visual distinction on bottom and maybe top, like a thin line
- [ ] BUG: Unsure how it occurred, but opened Candide.hearthx and moved several files to trash (all but chapter 1) and then deleted them all. Then when I imported a PNG, it opened in a tab but did not appear in the TreeView...
- [ ] ^ Added release logging, log viewer, and automatic Notebook TreeView expansion to help find this. It easily could have been an overlooked add-as-child, which didn't expand before
- [ ] One thing I think I would like to fix in the future is keeping file nesting but having operations like import or new file or whatever else resolve to the parent directory instead of the file, if the currently selected item is a file. This wound not apply to right click context menu actions
- [ ] Notebook TreeView multiple item selection, drag, delete, restore, etc
- [x] Replace QStringLiteral with u*_s?
- [x] ToString's kind of a hot mess
- [x] Re: `treeViews->setVisibilityKey(treeViewDockIniKey()); /// TODO TVT` in Notepad and Notebook (is a singular menu item in Workspace appropriate if it means we have to have such a silly virtual for each Workspace to override? At least, it seems silly to me right now)
- [ ] ^ Related: Right now, no way past having Ini::LocalKeys defaults in regular defaults function (because settings get command gets the default for all Ini keys internally - TreeViewService would need to be able to choose to use a localDefaults map or SettingsService could have a second command getter for Workspace-local settings...)
- [ ] Recovery prompts! We want just one big prompt with checkboxes for each recovered file (including Notebooks in with other file types (can have a visual cue to make them stand out))
- [ ] BUG: On Notepad recovery, there's a delay between displaying the first recovered tab and any subsequent recovered tabs (recovery prompt may or may not help)
- [ ] BUG: First time expanding a folder in Notepad TreeView takes a really long time
- [ ] Also, a Settings.md
- [ ] Remove settings INI info from Architecture.md?
- [ ] Add doc style doc
- [ ] Dialog overhaul - need a base for some of these (like a ScrollableDialog or Dialog with a central widget setter) and normalization between them
- [ ] Both the recovery dialogs and existing multi save prompt should use a path display dialog that displays parent directories as a collapsible header with checkbox beside it and each of its children (the files within)
- [x] HTML special plain-text type
- [ ] Styling for the focused indicator strip on bottom of plain text edit
- [ ] Also ^, there's a faint border area around QPlainTextEdit - currently, it is styled same as text edit background but might be nice to leave it "widget colored"
- [ ] Possible to make Workspace-modal dialogs (for something like close all windows or tabs)?
- [ ] ^ If so, do a modality overhaul, checking dialogs and call sites for what window or workspace to pass (or set app modal, like for quit)
- [ ] Backups prune cap setting
- [ ] Coco CMake?
- [ ] Have two "supported files" filters in addition to the "all files" (one for Notebook and one for Notepad which will also contain .hearthx)
- [ ] These filters should be set to list multiple extensions where applicable (meaning FileTypes needs an "all extensions" function to return canonical + aliases (JPG is the only one right now, when that's supported)
- [ ] Save As options could show a second (or third) filter (in addition to all files and/or all supported files) for the canonical type
- [ ] Need a function to return all extensions from the map (canonical + aliases)
- [ ] Maybe clean-up MagicBytes + FileTypes enums, make sure they're consistent (or, better yet, maybe they share an enum from a new header that holds an enum val for every type Hearth recognizes for any reason)
- [ ] Add a button to the open/import dialogs to navigate back to home (startDir)
- [ ] For Multi-file save prompt, can organize by directory, so we aren't repeating path for every file (just show parent as collapsible list and each individual name with check box (could also allow the list to be checked))

## File Types (Remaining)

From the file-types branch. See FileHandling.md for design details.

- [ ] NBX filter cleanup: Open Notebook, Save As Notebook, and Import dialogs still need `.hearthx` filters. Consider a Filters header/namespace pulling translatable names from Tr and extensions from `Nbx::Io::EXT` / `FileTypes`.
- [ ] Model/view re-evaluation on rename: when a file's extension changes, the system should swap to the appropriate model/view pair. Only affects Tier 2 types (special text), since Tier 1 (magic bytes) types are identified by content. Applies to both Notepad (filesystem rename) and Notebook (tree view rename).
- [ ] Consider NoOp for large unsupported binary files (e.g., images) where opening as text would be wasteful. Later-problem.
- [ ] NBX files within NBX archives: deliberately deferred. Nested archive lifecycle management conflicts with current architecture. See FileHandling.md for rationale.

## Features

- [ ] Spellcheck
- [ ] Find and replace
- [ ] Sessions for Notepad and Notebooks (Notepad sessions saved in User Data, Notebook in Archive Root)
- [ ] Pinned tabs (will utilize sessions)
- [ ] Tab groups (will utilize sessions, and maybe a better tab bar)
- [ ] Expanded/collapsed item restoration between sessions (will utilize sessions)
- [ ] Remembering dock position between sessions (using `QMainWindow::saveState()` / `restoreState()`)
- [ ] Status bar tools (AOT, Timer, Screen)
- [ ] Multiple TabWidgets per window for side-by-side
- [ ] Checkable export and compile feature with Dom tree
- [ ] Notebook TreeView multiple selections
- [ ] Notebook multiple selection export
- [ ] Line spacing options, if possible
- [ ] Hide menu bar (key to toggle)
- [ ] Notebook LRU cache for models, if needed

## Refinement

- [ ] Numbering for duplicate tabs (like Untitled:1 and Untitled:2, etc)
- [ ] Window Themes (theming infrastructure is in place, but window theming proving difficult (especially with tab bar, buttons, and collapsible widget header (see: window QSS template)))
- [ ] Ensure we are using terms around checkable items correctly in code (the term "toggle" is being used in a few different ways; checkable actions should be called checkables or similar)
- [ ] Make sure the distinction/usage between prompt, dialog, and box is clear
- [ ] Refactor common context menu items / TreeView hookup between Notepad and Notebook
- [ ] Notepad context menu (with rename, remove, collapse, expand, and anything else Notebook has that could apply)
- [ ] Renaming an open Notebook file in Notepad's TreeView: the Notebook works from its temp directory, so it is not broken immediately. The next save creates a file at the old path. No data loss, just a confusing orphaned file. Fix later.
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
- [ ] .hearthx Notebook icon
- [ ] Icon instead of trash text?
- [ ] Maybe stuff some of the closures in sub menus
- [ ] MenuBar highlight on hover, modeled after TabWidgetButton highlight (radius, color, etc.)
- [ ] Dock widget button/header styling

## Documentation

- [ ] Number and order for core docs
- [ ] Core docs should explain the program from start to finish (i.e., Openings.md, Modifications.md, Saves.md, Closures.md, etc.)
- [ ] Save prompts doc (preferred extension, how start paths are chosen/created)
- [ ] Saves of all types. Outline first everything that needs to be covered
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