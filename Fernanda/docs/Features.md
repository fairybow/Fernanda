# Features

Current version: v0.99.0-beta.18

## Separate Workspaces

Fernanda has two workspace types that share a common architecture (services, bus, hooks) but differ in how they manage files.

Shared capabilities:

- Multiple windows per workspace
- Open the same file in multiple tabs and windows (edits persist across all views)
- PDF and image viewing
- [Markdown](https://markdownguide.org/) and [Fountain](https://fountain.io) editing/viewing (live-rendered split preview)
- File imports (DOCX and RTF converted to plain text)
- Dockable Tree View
- Tiered settings (Notebook settings inherit from Notepad when unset, falling back to application defaults)
- Recovery autosave and automatic backups
- Hook-based closure system with save prompts at every level

### Notepad (single instance)

- Operates directly on the OS filesystem
- Tree View showing real directory structure
- Open, edit, and save any plain text file on disk
- Opening arguments support (double-click/drag a file in your OS to open it in Fernanda)
- TreeView file renaming
- File imports: DOCX and RTF converted to plain text and opened as unsaved tabs

> [!IMPORTANT]
> Known issue: Tree View root directory is locked in-place for now

### Notebook (0 or more instances)

- Archive-based workspace: all project content lives inside a single portable `.fnx` file (standard ZIP)
- Virtual directory structure via XML manifest (organize files and folders independently of physical storage)
- Create new Notebooks from scratch (name prompt, deferred first save)
- Open existing `.fnx` archives (extraction, validation, working directory setup)
- Multiple Notebooks can be open simultaneously
- Recoverable (standard ZIP format means content remains accessible outside Fernanda)
- File imports: any file can be imported into a Notebook (DOCX and RTF are converted to plain text; all other files are passed through with their original type preserved)
- Single file export (planned: multiple files, directories, as well as, eventually, full archive export/compilation)

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
- Configurable left/right margin (scales down with narrowing window widths)
- Word wrap modes (wrap anywhere, word boundary, no wrap, or smart)
- Undo/Redo
- Different editors showing the same text file will use separate layouts (wrap points)

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
- Import files into Notebooks (any type; convertible formats like DOCX and RTF become plain text)

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
- **Notebook**: two-tier save (individual file models saved to working directory, then archive compressed and written to `.fnx` path)
- Multi-file save prompts with checkable file list (select which files to save)
- Save As with path and name selection
- New Notebooks trigger Save As on first save
- Save failure reporting via dedicated message box listing failed files
- Modification tracking: DOM snapshot comparison for Notebooks, per-model tracking for Notepad
- Edited attribute management in Manifest.xml (set on edit, cleared before archive compression)
- Working directory rename handling on Save As

---

## Backups

- Automatic pre-overwrite backups for both Notepad and Notebook saves
- Notepad: backup created before each file write via `beforeWriteHook_`
- Notebook: backup created before archive compression via `BeforeOverwriteHook`
- Backups stored in `~/.fernanda/backups/notepad/` and `~/.fernanda/backups/notebooks/`
- Automatic pruning (oldest backups removed when cap is exceeded)

---

## Recovery Autosave

- Periodic flush of dirty buffers for crash recovery (15 second interval)
- Autosave never triggers backups, save prompts, tab indicators, or modification state changes
- Notebook: dirty files written to working directory, lockfile tracks FNX path, working directory, and dirty UUIDs
- Notepad: dirty files written to shadow recovery directory with buffer and metadata per file
- Recovery data cleaned up on successful save, discard, undo-to-clean, and clean exit
- On crash, orphaned recovery data persists and is detected on next launch
- Notebook recovery: lockfile scanning, working directory adoption, dirty state restoration
- Notepad recovery: on-disk files reopened with recovered buffers, off-disk files restored as untitled

---

## Tabs & Windows

- Tabbed interface with custom TabWidget (tab bar, close buttons, add-tab button)
- Tab drag-and-drop: drag tabs between windows or out to create new windows
- Tab drag visual: pixmap preview during drag
- Flagged tabs (visual indicator for modified files)
- Tab context menu: Duplicate, Save, Save As, Close, Close Everywhere
- Add-button context menu: file type selection (Plain Text, Markdown, Fountain)
- Multiple windows per workspace
- Window title updates reflecting current file and modification state
- Tab duplication (even for unsaved files)

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

> [!IMPORTANT]
> Known issue: Window themes not yet implemented

> [!TIP]
> User themes supported (place in `~/.fernanda/themes`), with hot reload. See an example theme [here](../resources/themes/Pocket.fernanda_editor)

---

## Settings

- Non-modal settings dialog with live preview
- Tiered/cascading settings: Notebook settings inherit from Notepad when unset, falling back to application defaults
- Per-Notebook settings stored inside the archive (`Settings.ini`)
- INI-based persistence via QSettings
- Settings organized into panels: Font, Themes, Key Filters, Editor, Word Counter, Color Bar

> [!IMPORTANT]
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
- Adaptive performance: instant updates for short documents, debounced updates for medium documents, click-to-refresh for very large documents (~500k+ characters)
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

> [!WARNING]
> System shutdown handling is implemented but untested

---

## Menus

- "New" submenu with file type options (Plain Text, Markdown, Fountain)
- Import action (workspace-specific filters and behavior)
- Close submenu (Close Tab, Close Tab Everywhere, Close Window Tabs, Close All Tabs)
- Notebook and Notepad operations (New/Open Notebook, Open Notepad) accessible from every workspace
- Menu state toggling based on current view, model, window, and workspace state

---

## Update Checker

- Check for updates via GitHub Releases API
- Button to open link to releases page

---

## File Type Detection

- Two-tier identification: magic bytes first (for binary formats), then extension matching (for text-based formats)
- Recognized binary signatures: PDF, PNG, JPEG, GIF, TIFF, BMP, WebP, ZIP (used for DOCX and FNX identification)
- Recognized text extensions: `.txt`, `.md`, `.markdown`, `.fountain`, `.html`, `.htm`, `.rtf`, and others
- Unrecognized files default to plain text
- File type registry (`Files.h`) centralizes extensions, type metadata, translatable names, dialog filter generation, and compound identification checks

---

## Infrastructure & Architecture

- Event-driven architecture with Bus system (commands + Qt signals)
- Service-based mechanics layer (WindowService, ViewService, FileService, TreeViewService, SettingsService)
- Module system for optional features (ColorBarModule, WordCounterModule)
- Hook-based policy injection (Services define decision points, Workspaces implement behavior)
- Two-phase initialization for safe cross-service communication
- Bus isolation per Workspace (no cross-contamination)
- Commander pattern for generic command dispatch (reusable, separable from Fernanda)
- Custom debug/logging system with `std::format` integration and custom Qt type formatters
- Prerelease dialog
- About dialog
- Translation infrastructure for future localization

---

## Planned

- Small utilities (like a pomodoro timer)
- FNX compilation/export
- Corkboard files (a slightly different approach to corkboards: standalone files that, when opened in Fernanda, display a corkboard and index cards that can be linked to existing files; would work in Notepad or Notebook)