# Features

Current vesion: v0.99.0-beta.1 "Bashō"

## Separate Workspaces

### Notepad (single instance)

- Operates directly on the OS filesystem
- Tree View showing real directory structure
- Open, edit, and save any plain text file on disk
- Open multiple files across multiple windows
- Open the same file in multiple places (with persisting edits)
- Opening arguments support (double-click/drag a file in your OS to open it in Fernanda)

> [!NOTE]
> Known issue: Tree View root directory is locked in-place for now

### Notebook (0 or more instances)

- Archive-based workspace: all project content lives inside a single portable `.fnx` file (standard 7zip)
- Virtual directory structure via XML manifest (organize files and folders independently of physical storage)
- Create new Notebooks from scratch (name prompt, deferred first save)
- Open existing `.fnx` archives (extraction, validation, working directory setup)
- Multiple Notebooks can be open simultaneously
- Recoverable (standard 7zip format means content remains accessible outside Fernanda)

---

## Editor

- Line numbers (gutter), configurable via settings
- Current line highlight with configurable color (stylable via editor themes)
- Selection handles (teardrop-shaped grabbable handles on text selections)
- Double-click whitespace selection (select runs of 2+ whitespace characters)
- Cursor blink restart on click for responsive feel
- Center-on-scroll option
- Overwrite mode toggle
- Configurable tab stop distance
- Word wrap modes (wrap anywhere, word boundary, no wrap, or smart)
- Undo/Redo

---

## Key Filters (Typing Enhancements)

- **Auto-close**: automatically insert matching punctuation pairs (brackets, quotes, etc.)
- **Delete-pair**: backspace between a matched pair removes both characters
- **Skip-closer**: typing a closing character when it's already ahead skips over it instead of doubling
- **Barging**: double-space jumps the cursor past closing punctuation; Enter barges past closers too
- **Trailing punctuation gap closing**: typing punctuation (comma, period, etc.) after a barge removes the extra space
- Smart (hopefully) handling of ambiguous quotes and contractions (apostrophes, backticks)
- All key filters individually toggleable in settings

---

## Notebook File Management

- Create files and virtual folders within a Notebook
- Rename files and folders inline (click-to-edit or F2)
- Drag-and-drop reorganization of the virtual directory tree
- Custom MIME type for internal drag operations
- Move-to-subtree prevention
- Context menus for file tree items (New File, New Folder, Expand/Collapse, Rename, Remove)
- Dirty indicator on modified Notebook files in the tree view
- Import files into Notebooks

### Trash System

- Soft-delete: items moved to a Trash section within the Notebook
- Trashed items remain editable with tabs still open
- Restore from trash to original parent (or root if parent was deleted)
- Permanently delete individual items or empty entire trash (with confirmation prompts)
- Drag-and-drop between main tree and trash (and back)
- Separate collapsible Trash tree view in an accordion layout below the main tree

---

## Saving

- **Notepad**: per-file saves to the OS filesystem, Save / Save As dialogs, preferred extension for untitled files
- **Notebook**: two-tier save — individual file models saved to working directory, then archive compressed and written to `.fnx` path
- Multi-file save prompts with checkable file list (select which files to save)
- Save As with path and name selection
- New Notebooks trigger Save As on first save
- Save failure reporting via dedicated message box listing failed files
- Modification tracking: DOM snapshot comparison for Notebooks, per-model tracking for Notepad
- Edited attribute management in Manifest.xml (set on edit, cleared before archive compression)
- Working directory rename handling on Save As

---

## Tabs & Windows

- Tabbed interface with custom TabWidget (tab bar, close buttons, add-tab button)
- Tab drag-and-drop: drag tabs between windows or out to create new windows
- Tab drag visual: pixmap preview during drag
- Flagged tabs (visual indicator for modified files)
- Tab text alignment (prevents leftmost text from clipping)
- Multiple windows per workspace
- Window title updates reflecting current file and modification state

---

## Tree View

- Dockable tree view panel for both Notepad and Notebook
- Notebook: dual tree views (main + trash) in accordion layout
- Notepad: filesystem browser with configurable columns and default widths
- Click empty space to deselect current selection
- Toggle tree view visibility (per-workspace, via menu and settings)
- Configurable default visibility per workspace type (Notepad off by default, Notebook on)

---

## Themes & Styling

- Editor themes (`.fernanda_editor` files; JSON-based with QSS template rendering)
- Theme selector in settings dialog with separate window and editor theme dropdowns
- Icon color support in window themes
- QSS template system with variable substitution

> [!NOTE]
> Known issue: Window themes not yet implemented

---

## Settings

- Non-modal settings dialog with live preview
- Tiered/cascading settings: Notebook settings inherit from Notepad when unset, falling back to application defaults
- Per-Notebook settings stored inside the archive (`Settings.ini`)
- INI-based persistence via QSettings
- Settings organized into panels: Font, Themes, Key Filters, Editor, Word Counter, Color Bar

> [!NOTE]
> Known issue: Notebook settings won't save unless the Notebook itself is saved

---

## Font Selection

- Font family dropdown with full system font list
- Bold and italic toggles
- Size slider with display
- Bundled fonts loaded at startup and prioritized at the top of the selection list
- Live preview on change

---

## Word Counter

- Line count, word count, character count (each individually toggleable)
- Selection-aware counting (shows selection counts alongside document counts)
- Selection replacement mode (replaces document counts with selection counts)
- Cursor position display: line number and column position
- Adaptive performance: instant updates for short documents, debounced updates for medium documents, manual refresh button for very large documents (~500k+ characters)
- All display elements individually toggleable in settings

---

## Color Bar

- Animated gradient progress bar for visual feedback (save success/failure, startup)
- Green for success, red for failure, pastel for startup
- Configurable position: top of window, below menu bar, above status bar, or bottom
- Auto-hide with timeline animation
- Toggleable in settings

---

## Closure & Lifecycle Management

- Hook-based closure system covering: close tab, close all matching tabs, close window tabs, close all tabs, close window, close all windows, and application quit
- Each closure level has workspace-specific policy (Notepad and Notebook handle closures differently)
- Save prompts at every closure level with Cancel/Save/Discard choices
- Model lifecycle management (close models, clean up views)
- Application quit routine: quit each Notebook, then Notepad, then application
- Passive quit when no windows are open

> [!IMPORTANT]
> System shutdown handling is implemented but untested

---

## Menus

- Menu state toggling based on current view, model, window, and workspace state
- Workspace-specific menus (Notepad menus vs. Notebook menus)
- Context menus for Notebook tree view items (main and trash)
- Menu builder system for declarative menu construction
- Notebook-specific items: Import, New File, New Folder, Open Notepad

---

## Update Checker

- Check for updates via GitHub Releases API
- Button to open link to releases page

---

## Installer (Windows)

- Inno Setup-based Windows installer
- Automated via batch file (`WindowsPackageRelease.bat`)
- Optional desktop shortcut
- Uses `windeployqt6` for Qt dependency bundling
- Separated output by platform

---

## File Type Detection

- Magic-byte-based file type detection (not extension-based)
- Recognized signatures: PNG, 7zip, RTF, PDF, GIF, JPG, ZIP/DOCX
- Unrecognized files default to plain text
- Non-text recognized types open as no-op (preventing binary display)

---

## Infrastructure & Architecture

- Event-driven architecture with Bus system (commands + Qt signal events)
- Service-based mechanics layer (WindowService, ViewService, FileService, TreeViewService, SettingsService)
- Module system for optional features (ColorBarModule, WordCounterModule)
- Hook-based policy injection — Services define decision points, Workspaces implement behavior
- Two-phase initialization for safe cross-service communication
- Bus isolation per Workspace (no cross-contamination)
- Commander pattern for generic command dispatch (reusable, separable from Fernanda)
- Custom debug/logging system with `std::format` integration and custom Qt type formatters
- Prerelease beta alert dialog
- About dialog
- Translation infrastructure (TR system) prepared for future localization

---

## Technical Details

- C++ / Qt 6.9.2+ (Qt 6.10.1)
- Windows x64 (primary target)
- bit7z (rikyoz) for 7zip archive handling
- Custom Coco utility library (paths, concepts, debug, utilities)
- Mostly header-only development for now (for my own readability)
- GPL 3 licensed with additional terms under Section 7

---

## Planned

- Auto-save
- Automatic back-ups
