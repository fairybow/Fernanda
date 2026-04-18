# Changelog

[Skip to release content](#releases)

---

# Boilerplate

(See [build_release_text.py](https://github.com/fairybow/Fernanda/blob/main/.github/scripts/build_release_text.py))

<!-- release-preamble-start -->

**Fernanda is a plain-text-first workbench for creative writing.** Work on single files like a notepad or organize whole projects in Notebooks (`.fnx`).

This is a soft release. For a full feature list, see [Features.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/Features.md). For past release details, see [CHANGELOG.md](https://github.com/fairybow/Fernanda/blob/main/CHANGELOG.md)

> [!WARNING]
> You should not trust your writing with any version of this software less than 1.0.0! Regardless, always make regular backups of your work.

<!-- release-preamble-end -->

<!-- release-footer-start -->

## Installation

**Windows:** Download and run the installer below.

**macOS:** Download the `.dmg`, open it, and drag Fernanda to Applications.

**Linux:** Download the `.AppImage`, make it executable (`chmod +x`), and run.

> [!NOTE]
> **Windows:** This build is unsigned, so Windows Defender SmartScreen will likely show a warning. Click **More info -> Run anyway** to proceed.
>
> **macOS:** This build is unsigned. You may need to right-click and select **Open** the first time, or allow it in **System Settings -> Privacy & Security**.

## Updating

**Windows:** Download the newest installer, ensure Fernanda is closed, then run the installer and install to the same directory (default `C:/Program Files`), overwriting.

**macOS:** Replace the app in Applications with the new version from the `.dmg`.

**Linux:** Replace the old `.AppImage` with the new one.

## Uninstalling

**Windows:** Run the uninstaller (`unins000.exe`) or remove via **Add or Remove Programs** as usual.

**macOS:** Drag Fernanda from Applications to the Trash.

**Linux:** Delete the `.AppImage`.

Fernanda doesn't delete its data folders on uninstall since they may contain writing. You may want to remove them manually:

| Folder | Location | Contents |
|---|---|---|
| User Data | `~/.fernanda/` | Settings, backups, temp files, themes |
| Default Docs | `~/Documents/Fernanda/` | Default location for file dialogs |

## Platforms

Windows (x64), macOS (ARM), and Linux (x86_64).

> [!NOTE]
> macOS and Linux builds are available but not well-tested. Bug reports are welcome!

:heart:

<!-- release-footer-end -->

---

# Notes

- For release note links: use `blob/main` for evergreen links; use `blob/<tag>` for links to a specific release's snapshot!
- For diffing against previous release: `https://github.com/fairybow/Fernanda/compare/<tag>...main.diff`
- Release commands:
```
git tag v0.99.0-beta.24 [must match tag part of entry title (see below)]
git push origin v0.99.0-beta.24
```

---

<a id="releases"></a>

# 0.99.0-beta.25 (Testing / Soft Release) - tag v0.99.0-beta.25

## What's New?

**Custom format machinery.** Replaced the `std::format`-based ecosystem (Formatters.h + ToString.h) with a new Fmt::format built on `QString`/`QStringView`, plus a consolidated ToQString.h under a new fmt/ directory. The new log formatting is less flexible but does all we need it to (just curly brace replacements, no specifiers)

**Build-time debug flag.** VERSION_DEBUG now comes from CMake instead of relying on MSVC's `_DEBUG`, so Debug builds behave consistently across Windows, macOS, and Linux

**ImageFileView Fit-mode scroll bar gap fix.** Scroll bars are now disabled in Fit mode, so switching zoom modes and back no longer leaves a scroll-bar-sized gap

**Unique-tabs focus refinement.** When unique tabs is on and the requested model is already the active view, the tab widget now re-focuses instead of searching from scratch for the requested model (and possibly activating a different tab)

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Session restore not yet implemented (splits, open tabs, window positions, expanded Notebook TreeView items, etc.)

---

# 0.99.0-beta.24 (Testing / Soft Release) - tag v0.99.0-beta.24

## What's New?

**Minor adjustments.** Adding missing tab closure options to tab context menus, as well as restructuring that and the "File" menu

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- ImageFileView Fit-mode sizing doesn't account for scroll bar width (noticeable after switching zoom modes and back)
- Session restore not yet implemented (splits, open tabs, window positions, expanded Notebook TreeView items, etc.)

---

# 0.99.0-beta.23 (Testing / Soft Release) - tag v0.99.0-beta.23

## What's New?

**Tab splits.** TabWidgets can now be arranged side-by-side within a Window. Move or duplicate the active tab left/right via the File > Split submenu or tab context menu, or drag a tab near the left/right edge of a TabWidget to drop it into a new split there. Empty splits auto-collapse. New [Splits.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/Splits.md) doc

**Unique tabs option.** Per-workspace toggle (View menu). When enabled, opening an already-open file raises its existing tab instead of creating a duplicate. Defaults on. Implemented via a new `shouldOpenTabHook` on `ViewService` (tabs can still be duplicated while this mode is on via menus and turning this mode on will not delete existing duplicates)

**New `TabSurface` widget.** The Window's central widget is now a `TabSurface` that owns a horizontal `QSplitter` of `TabWidget`s and tracks the active split via focus changes. `ViewService` was split into header + source and restructured around multiple TabWidgets per window: queries, tab ops, and helpers all iterate splits, and close-tab/close-everywhere now operate on models rather than `(window, index)` pairs

**Hook signature changes.** `canCloseTab` and `canCloseTabEverywhere` now take `(Window*, AbstractFileModel*)` instead of `(Window*, int index)`. New `canCloseSplit(Window*)` hook. New `Bus::splitCountChanged(Window*)` signal for menu state refresh

**Workspace local Ini keys consolidated.** `Notepad` and `Notebook` now pass a `LocalIniKeys` struct to the `Workspace` base constructor (currently tree view dock visibility + unique tabs), replacing the `treeViewDockIniKey()` virtual

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- ImageFileView Fit-mode sizing doesn't account for scroll bar width (noticeable after switching zoom modes and back)
- Session restore not yet implemented (splits, open tabs, window positions, expanded Notebook TreeView items, etc.)

---

# 0.99.0-beta.22 (Testing / Soft Release) - tag v0.99.0-beta.22

## What's New?

**Image viewer rewrite.** `ImageFileView` now uses a `QGraphicsView`-based renderer instead of a `QLabel` in a `QScrollArea`. Images can be panned by click-dragging and zoomed with the scroll wheel. GIF animation works the same way (frame-by-frame pixmap updates to the graphics item)

**Zoom state extracted.** `ZoomControl` no longer owns zoom mode/factor logic. A new `ZoomState` value type holds mode (Fit/Fixed), factor, and step/clamp math. Views own their `ZoomState`, mutate it in response to `ZoomControl` signals, then push display text back

**Icons extracted from StyleContext.** SVG icon rendering moved to a standalone `Icons` namespace and `UiIcon` enum in Icons.h. `StyleContext` still provides the themed-and-cached API for widgets, but the registry and render logic are no longer its responsibility. Zoom control buttons now use SVG icons instead of text glyphs

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale

---

# 0.99.0-beta.21 (Testing / Soft Release) - tag v0.99.0-beta.21

## What's New?

**WebEngineView resize masking.** The stutter-hiding overlay that previously lived in AbstractMarkupFileView (and only covered markup previews) moved into WebEngineView itself. This means HtmlFileView now also masks visual stutter on resize and initial load

**Improved info tooltips.** Settings field info icons now use a custom popup with a shorter hover delay (400ms) instead of the native QToolTip, and clicking the icon locks the popup visible for 3 seconds, so they're easier to read/activate

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet
- ImageFileView's scroll bars are entirely black

---

# 0.99.0-beta.20 (Testing / Soft Release) - tag v0.99.0-beta.20

## What's New?

**Notebook tree view icons.** Files in the Notebook tree now show type-appropriate icons (Markdown, PDF, images, HTML, etc.) using bundled Breeze icons (LGPL), with a plain text fallback for unrecognized types (this is how Fernanda currently treats all unrecognized types, as plain text). Icons are based on extension, so files with improper extensions will display the wrong icon but still open appropriately if they have [magic bytes (a file signature)](https://en.wikipedia.org/wiki/List_of_file_signatures). (Improper opening is unavoidable for special plain text cases like Fountain which can only be distinguished by extension)

**HTML file viewing.** Saved HTML files can now be opened as read-only rendered pages (via `QWebEngineView`). Useful for saving research material in a Notebook (though they'll need to be saved from the OS browser and manually imported for now)

**ImageFileModel -> RawFileModel.** This model now serves as a generic byte-bag for any non-text, non-PDF format (images, HTML)

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.19 (Testing / Soft Release) - tag v0.99.0-beta.19

## What's New?

**Upping Qt 6.10 (a GitHub CI release workflow tmpoerary reversion) to 6.11.**

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

## This Version's Dumbest Code Award :trophy:

For once, it won't be me. This one goes to libc++ (see [Debug.h](https://github.com/fairybow/Fernanda/blob/c4b1643350d87f423f4e85552b58d2b212ffff95/Fernanda/src/core/Debug.h)).

---

# 0.99.0-beta.18 (Testing / Soft Release) - tag v0.99.0-beta.18

## What's New?

**macOS and Linux builds.** Now shipping macOS (ARM) and Linux (x86_64) installers alongside the Windows build. (These are the first cross-platform releases and have not been extensively tested)

**File imports.** Both workspaces now share an "Import..." action in the File menu. Notepad imports convert supported formats (DOCX, RTF) to plain text and open them in new tabs. Notebook imports do the same for convertible formats and pass through everything else into the archive

**Tab context menu.** Right-clicking a tab now shows Duplicate, Save, Save As, Close, and Close Everywhere. Right-clicking the "+" button shows the creatable file types (Plain Text, Markdown, Fountain)

**Editor left/right margin setting.** A new slider in Editor settings adds symmetric horizontal margins to the text viewport. The margin scales down when the editor is narrow (below 800px)

**Menu reorganization.** The Notebook-specific "Notebook" menu is gone. "Open notepad" moved into the shared File menu (hidden when already in Notepad). File creation, import, and notebook operations now live in a unified File menu structure across both workspace types. Close actions grouped into a "Close" submenu

**Notebook color chip improvements.** Chips now update live on Save As

**CI release workflow.** Releases are now built and packaged automatically via GitHub Actions

**Linux packaging support.** Added `FERNANDA_USE_SYSTEM_LIBS` CMake flag for Linux packagers to build against system-installed md4c and miniz instead of bundled submodules. Icons, translations, and a `.desktop` file now install to standard FHS paths on Linux. Translation load path in `Application.h` falls back to the FHS location on Linux. Version.txt dropped from install (build artifact only). (Closes #131)

**`FileTypes` renamed to `Files`.** `FileTypes.h` is now `Files.h`; `Kind` is now `Type`. The namespace also absorbs FNX-file and DOCX-file compound identification checks (extension + magic bytes), dialog filter generation (`Files::filters()`), and translatable type names (via `Tr.h`). `Fnx::Io::isFnxFile` and `Fnx::Io::EXT` removed in favor of `Files::isFnxFile` and `Files::canonicalExt(Files::Notebook)`

**FNX archive safety.** `Fnx::Io::decompress` and `compress` now use `qScopeGuard` for miniz cleanup, preventing leaks on early return. The compress path explicitly closes the writer before file operations

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.17 (Testing / Soft Release) - tag v0.99.0-beta.17

## What's New?

**New file type options.** Both Notepad and Notebook now have a "New" submenu (under the existing "New tab" / "New file" actions) with options for Plain Text, Markdown, and Fountain

**Recovery preserves file type.** Notepad's recovery entries now store and restore file kind, so recovering an unsaved Markdown or Fountain file reopens it in the correct mode instead of defaulting to plain text

**Tab title strips markup syntax.** Tab titles derived from file content now strip leading Markdown heading markers (`#`) and Fountain `Title:` prefixes, showing just the title text

**Bundled font data centralized.** Font family names, CSS `@font-face` rules, and editor size limits are now all in `BundledFonts.h`. `FontPanel` now shows the default font family (mononoki) at the top of the picker, separated from other bundled and system fonts

**`Command` unused `Window` context removed.** `Command` no longer carries a `Window*` context field

**`MenuBuilder` submenu support.** `MenuBuilder` gains `submenu()` and `endSubmenu()` for nested menus

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.16 (Testing / Soft Release) - tag v0.99.0-beta.16

## What's New?

**Notebook color chips.** Each Notebook window now displays a colorful "chip" in the status bar with its name for easier window domain recognition. The initial color is deterministically generated from the name but it can be customized via a right-click context menu. Custom colors persist in the Notebook's local settings

**Notebook file/folder rename on add.** Newly added Notebook files and folders will automatically open their line edits for renaming

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.15 (Testing / Soft Release) - tag v0.99.0-beta.15

## What's New?

**Markup preview mode switch.** The cycling button is now a segmented `MultiSwitch` widget (Edit / Split / Preview) with an animated sliding highlight pill.

**Markup preview polish.** The first-ever `QWebEngineView` load flicker (noted in beta.14) *should* now be masked by a new, dedicated warm-up overlay

**Word counter click-to-refresh.** The "Refresh" button for large documents is gone. Instead, when auto-count is disabled (500k+ chars), the counts display dims to indicate staleness, and clicking anywhere on the word counter triggers a manual recount. Selection counts remain bright while a selection is active. `WordCounter` moved from `modules/` to `ui/`

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Image/PDF zoom: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.14 (Testing / Soft Release) - tag v0.99.0-beta.14

## What's New?

**Markup preview.** Markdown and [Fountain](https://fountain.io) (screenplay) files now open in a split editor with a live-rendered preview panel. A mode toggle cycles between Split, Edit, and Preview. The preview uses incremental DOM patching: on each reparse, only changed blocks are updated via JavaScript `outerHTML` assignment, with a full `innerHTML` fallback when block count changes. Rendering is powered by `QWebEngineView` (Chromium) for full CSS support. `QWebEngineView`'s unavoidable visual stutter in transitional states and during resizing is mostly masked via utility overlay widgets (with the exception of an initial flicker on its very first use)

**New dependencies.** [md4c](https://github.com/mity/md4c) (v0.5.2, C Markdown parser) and [fountain.h](https://github.com/fairybow/fountain.h) (C Fountain parser/renderer). Qt WebEngine is now required (along with Qt Position and Qt WebChannel). CMake project language list expanded to include C (C99)

**Courier Prime bundled.** The Courier Prime font family ships with Fernanda (available in the font picker and used as the display font for Fountain preview)

**File type detection at model creation.** `TextFileModel` now resolves its `FileTypes::Kind` from the file path at construction (via `FileTypes::fromPath`) instead of always defaulting to `PlainText`. Added `.markdown` (alongside existing `.md`), `.html`, and `.htm` extensions to `FileTypes`

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet
- Markup file previews flicker on very first load

---

# 0.99.0-beta.13 (Testing / Soft Release) - tag v0.99.0-beta.13

## What's New?

**Notebook UI.** Parent items (folders or files) now display "(*)" when any descendant file is modified.

**Settings improved.** `GET_SETTING` Bus command resolves defaults automatically (no passing `defaultValue` at every call site anymore)

**Code cleanup.** General reduction in repeated ViewService code (`forEachTabOfModel_`, `forEachFileView_<ViewT>`, `indexOfModel_`, and `TextViewSetting_` replace large blocks of duplicated iteration and applier registration). Additionally, instead of `QVariantMap`, `Ini` uses `Ini::Map` and Bus commands use `Command::Params` for readability and to avoid confusion (both are aliases for `QHash<QString, QVariant>`, a.k.a. `QVariantHash`)

**Added CONTRIBUTING.md and corresponding README.md section.**

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.12 (Testing / Soft Release) - tag v0.99.0-beta.12

## What's New?

**Log files and log viewer.** Debug now writes to `~/.fernanda/logs/` (info level by default; debug level with `--verbose`). Logs are pruned automatically. A `--log-viewer` flag opens an in-app log viewer window for the session.

**Notebook TreeView improvements.** After adding a new file, folder, or importing files (or restoring from trash), the parent is now expanded and the new item is selected automatically.

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.11 (Testing / Soft Release) - tag v0.99.0-beta.11

## What's New?

**Recovery autosave.** Both workspaces now periodically flush dirty buffers to crash recovery locations. Notebook writes to its working directory and maintains a lockfile tracking the FNX path and dirty UUIDs. Notepad writes per-file buffers and metadata to a shadow recovery directory. Orphaned recovery data is detected on next launch and can be restored. Recovery data is cleaned up on save, discard, undo-to-clean, and clean exit. (See [RecoveryAutosave.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/RecoveryAutosave.md).)

**`TempDir` replaced by `WorkingDir`.** Notebook's working directory is no longer a `QTemporaryDir` wrapper. It persists through crashes by design. `WorkingDir` tracks whether it was freshly created or adopted from an orphaned path, which drives the recovery path in `setup_()`.

**`AppDirs` on-demand creation.** Directories are now created on first access rather than requiring an upfront `initialize()` call.

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.10 (Testing / Soft Release) - tag v0.99.0-beta.10

## What's New?

**Pre-save backups.** Before saving, Fernanda now copies the original to `~/.fernanda/backups/`. Backups are pruned to a cap (currently hardcoded at 5). New `Backup.h` namespace handles creation, naming (`{hash}_{stem}.{timestamp}{ext}`), and pruning. Notepad hooks via `FileService::beforeWriteHook` and Notebook hooks via `Fnx::Io::BeforeOverwriteHook` passed to `Fnx::Io::compress()`.

**Fix: Intermittent save failure.** `QFileSystemWatcher` could hold a transient handle during `QSaveFile::commit()`, causing "Access is denied." `writeModelToDisk_` now removes the path from the watcher before writing and re-adds it after.

**Expanded `AppDirs`.** New subdirectories for backups, notebook temps, and future recovery paths. Added `cleanup()` called from `~Application()` to remove empty temp/recovery dirs on exit.

**Removed ADDITIONAL_TERMS.**

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet
- No auto-save/recovery!

---

# 0.99.0-beta.9 (Testing / Soft Release) - tag v0.99.0-beta.9

## What's New?

> [!WARNING]
> Breaking changes

**Replaced bit7z (7-Zip) with miniz (ZIP).** FNX files are now standard ZIP archives (just like DOCX, EPUB, and others), not 7-Zip. This eliminates runtime FNX read/write dependencies, reduces the application's footprint, and quickens build times. Old FNX files will not open correctly but are still recoverable with [7-Zip](https://www.7-zip.org/).

*This should be the last breaking change related to FNX files.*

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet
- No auto-save, no save back-ups!

---

# 0.99.0-beta.8 (Testing / Soft Release) - tag v0.99.0-beta.8

## What's New?

**Notebook trash widget fix.** New `DrawerWidget` for a simple, splitter-aware collapsible panel.

**Cross-platform groundwork.** Added cross-platform `Workspace` window-raising code (`XPlatform::stackUnder`), plus other cross-platform code changes.

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet
- No auto-save, no save back-ups!

---

# 0.99.0-beta.7 (Testing / Soft Release) - tag v0.99.0-beta.7

## What's New?

**Stale files are now treated as modified.** When a file disappears from disk (deleted, moved, or unmounted), Fernanda marks it modified so the user gets a save prompt before closing. In Notepad, saving a stale file routes to Save As instead of silently failing.

**Renamed `supportsModification`/`supportsEditing` to `isUserEditable`.** This clarifies the difference between "the user can edit this" and "this file has changed." A PDF whose backing file is deleted is modified (needs saving) but not editable (can't type into it). Modification tracking moved to the AbstractFileModel base class so any model type can participate.

**Consistent path display in save/reload prompts.** All prompts now use `Coco::Path::prettyQString()` so paths look the same everywhere.

**Tab alerts survive drag-and-drop.** Alert state (like "file modified externally") is now preserved when dragging tabs between windows.

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet
- No auto-save, no save back-ups!

---

# 0.99.0-beta.6 (Testing / Soft Release) - tag v0.99.0-beta.6

## What's New?

**Open file change detection.** `FileService` now owns a `QFileSystemWatcher` monitoring all on-disk file model paths. When a file is modified externally, `Bus::fileModelExternallyModified` fires and `ViewService` shows a `ReloadPrompt`: the user can reload from disk or keep their in-memory version. When a file disappears (deleted, moved, or volume unmounted), `Bus::fileModelPathInvalidated` fires and the tab gets a persistent alert. Regular saves via `Io::write` are suppressed with a `recentlyWritten_` set so Fernanda's own saves don't trigger the prompt.

**FileMeta staleness.** `FileMeta` now tracks an `isStale_` flag (set when the on-disk path vanishes, cleared on `setPath()`). Stale files show "[Stale]" appended to their path in the tooltip, and `FileService::save()` refuses to write them.

**Notepad drag-and-drop path tracking.** `NotepadFileSystemModel` now overrides `dropMimeData` to snapshot source paths before delegating to the base class, then emits `fileMoved` for each successfully moved file. Notepad connects to this (alongside the existing fileRenamed connection) to keep FileMeta paths current, and the `QFileSystemWatcher` path swap follows automatically.

**Tab alert widget redesign.** `TabWidgetAlertWidget` is now created on demand (instead of always-present-but-hidden) and deleted on `clearTabAlert`. The icon changed from a dark red circle to a yellow warning triangle (better visibility/accessibility).

## Known Issues

- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.5 (Testing / Soft Release) - tag v0.99.0-beta.5

## What's New?

**Deferred close coalescing.** `Window::closeEvent` now always defers to `WindowService::deferClose_()` instead of calling `canCloseHook_` immediately. A zero-delay timer coalesces all close events from the same tick: one pending window takes the normal single-window path, multiple pending windows route through `closeAll()`. This fixes the nested event loop / "Don't Save does nothing" bug when the OS closes all windows at once.

**Tab drag fix.** `dragPressIndex_` is now recorded on mouse press rather than recalculated on mouse move. This prevents the wrong tab from being dragged when the user has reordered tabs before initiating a drag. The index is also tracked through tabMoved events to stay accurate if Qt reorders tabs during the move.

## Known Issues

- Notepad TreeView allows moving files, which means FileMeta paths can become stale
- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

---

# 0.99.0-beta.4 (Testing / Soft Release) - tag v0.99.0-beta.4

## What's New?

**Image viewing.** `ImageFileModel` and `ImageFileView` added for PNG, JPEG, GIF, TIFF, BMP, and WebP. Like PDFs, images are detected by magic bytes. View-only, with fit-to-view default. Save As exports the raw bytes. (Images, like any other file, can be imported into Notebooks.)

**Zoom controls.** `ZoomControl` is a floating overlay widget anchored to the bottom-right of its parent, shared by both `PdfFileView` and `ImageFileView`. Left-click the display to toggle between Fit and the last used fixed zoom; right-click to reset to 100%. Plus/minus buttons step in 10% increments.

**CMake migration.** The build system has been switched from `.vcxproj` / `.sln` to CMake. `Version.txt` is now generated at configure time instead of by PowerShell pre-build script. bit7z is now a Git submodule ([fork](https://github.com/fairybow/Fernanda-bit7z) pinned to v4.0.11) built from source via `add_subdirectory()`, replacing the pre-built `.lib` files.

### Other

- Workspaces now track a rolling last-used directory for open/import dialogs
- `FileMeta` tooltips now show title, path (or "[Not on disk]"), and file type name instead of just the raw path
- `FileTypes` expanded with TIFF, BMP, and WebP; added `fromMagicBytes()` converter and `name()` for human-readable type names
- Source directory restructured from flat `src/` into subdirectories (`core/`, `dialogs/`, `fnx/`, `menus/`, `models/`, `modules/`, `services/`, `settings/`, `ui/`, `views/`, `workspaces/`); all include paths updated; third-party includes now use angle brackets
- Lambda captures changed from `[&]` to explicit `[this]` (or `[this, ...]`) throughout the codebase
- Translation QM files now loaded from the application directory at runtime instead of embedded via QRC

## Known Issues

- Notepad TreeView allows moving files, which means FileMeta paths can become stale
- TreeView root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum
- Zoom controls: no scroll/content position realignment on zoom change yet; no panning support yet

## This Version's Dumbest Code Award :trophy:

This shorthand in [Coco](https://github.com/fairybow/Coco/blob/main/Coco/include/Coco/Utility.h):

```
#define qVar(T) QVariant::fromValue(T)
```

(But I'm keeping it, and there's nothing anyone can do about it!)

---

# 0.99.0-beta.3 (Testing / Soft Release) - tag v0.99.0-beta.3

## What's New?

**PDF viewing.** `PdfFileModel` and `PdfFileView` added using Qt's `QPdfView` (multi-page, fit-to-width). PDFs are detected by magic bytes, so the file extension doesn't matter. View-only for now, but Save As works (exports the raw bytes). PDFs can be imported into Notebooks.

**`Notebook` file export.** Files can now be exported from both the main tree view and trash context menus. The suggested filename on export is reconstructed from the display name plus the stored extension (e.g., `Chapter One.txt`).

**`Notepad` file renaming via `TreeView`.** The tree view now allows renaming files inline (selected-click or F2). Directory renaming is blocked for now (as this is more complicated, re: informing all potentially open children of their changed path). If the renamed file is currently open, its path is updated.

### File type handling architecture

tl;dr: there was a fairly substantial overhaul of how Fernanda identifies and routes file types.

- The old FileTypes.h (magic bytes detection) has been split: magic byte logic moved to a new MagicBytes.h; FileTypes.h is now a central registry mapping enum values to canonical extensions
- `FileService` now uses two-tier resolution, with magic bytes first (binary formats like PDF), then extension matching (for future special text types like Markdown, Fountain), with universal plain text fallthrough (anything that isn't anything else is plain text to Fernanda)
- `AbstractFileModel`'s contract has been reworked. `data()` and `setData()` are now the only pure virtuals (each subclass owns its storage); `supportsModification()` is a regular virtual defaulting to false
- `FileMeta` now stores `FileTypes::Kind` and provides `preferredExt()` (on-disk extension if the file exists; canonical extension otherwise). `preferredExtension()` removed from model subclasses
- `FileService::saveAs` no longer guards on `supportsModification()`: every model has `data()`, so every model can be exported
- FNX import generalized from text-only to any file type. `importTextFile` -> `importFile` (extension taken from source path); `addNewTextFile` -> `addNewFile(FileTypes::Kind)`
- FNX manifest version bumped from 1.0 to 1.1
- `NoOpFileModel`/`NoOpFileView` retired. Everything now falls through to plain text (no-op model/views may potentially return for blocking wasteful binary display cases (e.g., opening a large, unsupported image format))
- `TieredSettings` key converters added for making certain INI values human-readable instead of `QVariant` byte arrays. `setKeyConverters()` on `TieredSettings` lets us register per-key serialize/deserialize functions
- File tab tooltips now use `path_.prettyQString()` instead of the raw path string (meaning they won't contain mismatched slashes and will be uniform)

### Other

- Installer: Added `[InstallDelete]` section that clears the previous install's data directory before copying new files, and added `CloseApplications`/`CloseApplicationsFilter` to prompt closing Fernanda before installing. Adds ignoreversion flags to file entries
- Qt updated to 6.10.2 (from 6.10.1); pdfwidgets module added to project
- Coco submodule updated: `PathUtil` namespace merged into `Coco` namespace throughout (`Coco::PathUtil::mkdir` -> `Coco::mkdir`, `Coco::PathUtil::copy` -> `Coco::copy`, dialog helpers, filePaths, paths, findParent, etc.); additionally, `Path`'s string caching fully reworked
- `ControlField`'s initializations are now less crazy
- `ControlField` info option now draws a custom icon instead of `QStyle::SP_MessageBoxInformation`
- Debug.h: static functions changed to inline
- `Application`'s arg parsing drastically simplified

## Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)
- Renaming an open Notebook's `.fnx` file in Notepad's TreeView can cause the Notebook's save target to go stale
- Trash splitter handle behavior: clicking the handle alone can size the closed state up; trash view can't be shrunk below its minimum

## This Version's Dumbest Code Award :trophy:

`Application`'s byzantine arg parsing.

We receive args in `Application` at two points: on start-up and when a relaunch is attempted (see [`Coco::StartCop`](https://github.com/fairybow/Coco/blob/main/Coco/include/Coco/StartCop.h)). Previously, it went a little something like this:

```cpp
void handleArgs_()
{
    auto parsed_args = parseArgs_(arguments());

    if (parsed_args.isEmpty()) {
        // Open Notepad single empty window, show color bar pastel
        notepad_->show();
        notepad_->beCute();

    } else if (parsed_args.hasOnlyFnx()) {
        // Make a Notebook for each FNX file, open single window for each,
        // show color bar pastel in each Notebook's window
        for (auto& path : parsed_args.fnxFiles) {
            if (auto notebook = makeNotebook_(path)) {
                notebook->show();
                notebook->beCute();
            }
        }
    } else if (parsed_args.hasOnlyRegular()) {
        // Open Notepad single window with a tab for each file, show color
        // bar pastel
        notepad_->show();
        notepad_->openFiles(parsed_args.regularFiles);
        notepad_->beCute();
    } else {
        // Has both:
        // - Open Notepad single window with a tab for each file, show color
        // bar pastel
        // - Make a Notebook for each FNX file, open single window for each,
        // show color bar pastel in each Notebook's window
        notepad_->show();
        notepad_->openFiles(parsed_args.regularFiles);
        notepad_->beCute();

        for (auto& path : parsed_args.fnxFiles) {
            if (auto notebook = makeNotebook_(path)) {
                notebook->show();
                notebook->beCute();
            }
        }
    }
}
```

Here's the revision:

```cpp
void handleArgs_()
{
    auto parsed = parseArgs_(arguments());

    // Show notepad if we have regular files or nothing at all
    if (!parsed.regularFiles.isEmpty() || parsed.fnxFiles.isEmpty()) {
        notepad_->show();
        notepad_->openFiles(parsed.regularFiles); // No-op if empty
        notepad_->beCute();
    }

    // Make a Notebook for each FNX file (and open single window for each)
    for (auto& path : parsed.fnxFiles)
        openNotebook_(path);
}
```

(See [Application.h (3f8bad1)](https://github.com/fairybow/Fernanda/blob/3f8bad164850dead66261a2f5c4acae1bd5fd1ab/Fernanda/src/Application.h))

Runner-up:

`ControlField`'s ridiculous initializations:

```cpp
ControlField<DisplaySlider*>* tabStopDistance_ =
        new ControlField<DisplaySlider*>(
            ControlField<DisplaySlider*>::Label,
            this);
ControlField<QComboBox*>* wrapMode_ = new ControlField<QComboBox*>(
    ControlField<QComboBox*>::LabelAndInfo,
    this);
```

^ lol

(See [EditorPanel.h (a4d94df/v0.99.0-beta.2)](https://github.com/fairybow/Fernanda/blob/a4d94dfd7a71fa2a39ad22d28fa0fef3ca7e534f/Fernanda/src/EditorPanel.h))

---

# 0.99.0-beta.2 (Testing / Soft Release) - tag v0.99.0-beta.2

## What's New?

- Settings pipeline overhaul: new `SettingsPanel` base class replaces per-panel boilerplate; `SettingsDialog` emits a single `settingChanged(key, value)` signal instead of around 20 individual ones; panels take `QVariantMap` instead of custom `InitialValues` structs; debouncers unified into a `QHash`
- "Prime Document" hack to allow multiple editors showing the same file to have separate layouts (which means, fundamentally, they need separate documents instead of sharing the same document)
- New documentation: `PrimeDocument.md`
- `MenuBuilder` decoupled from `Window` (takes `QWidget*` parent instead of `Window*`); gains `enabled()` builder method
- Fatal messages now always print regardless of logging toggle; `Debug::print` uses `QMessageLogger` instead of calling handler directly
- `KeyFilters` compound edit support via `MultiStepEditScope_` RAII guard and `multiStepEditBegan`/`multiStepEditEnded` signals (coordinates with prime document delta routing)
- New key filter: trailing punctuation gap closure on Enter
- Tab duplication (even for unsaved files)

## Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)

## This Version's Dumbest Code Award :trophy:

A user toggles Line Numbers in the settings dialog. A `bool` needs to travel from a checkbox to the editor. So:

**Step 1**: The checkbox emits `toggled`. The panel catches it and emits `lineNumbersChanged`.

**Step 2**: The `SettingsDialog` catches `lineNumbersChanged` and emits `editorLineNumbersChanged`.

**Step 3**: The `SettingsService` catches `editorLineNumbersChanged`, persists the value, and emits a Bus event with a string key.

**Step 4**: The `ViewService` catches the Bus event and compares the key against 13 other keys, one `if` at a time (not `else if`, every branch, every time), until it finds the match and finally calls `setLineNumbers(true)`.

Four signal emissions, four connections, and a string-keyed dispatch resolved by brute-force linear scan. And this entire journey is copy-pasted roughly 20 times, once per setting. The `SettingsDialog` alone declares ~20 signals whose sole purpose is to hear a panel signal and re-emit it verbatim.

```cpp
// SettingsDialog.h: one of about 20 identical relays
connect(
    editorPanel_,
    &EditorPanel::lineNumbersChanged,
    this,
    [&](bool lineNumbers) {
        emit editorLineNumbersChanged(lineNumbers);
    });
```

```cpp
// ViewService.h: one of 13 if blocks
if (key == Ini::Keys::EDITOR_LINE_NUMBERS) {
    auto v = value.value<bool>();
    forEachTextFileView_(
        [&](TextFileView* view) { view->editor()->setLineNumbers(v); });
}
```

(See [SettingsDialog.h (a33c4be)](https://github.com/fairybow/Fernanda/blob/38cf4be87281fbc12950a3cba979f6555d67cbca/Fernanda/src/SettingsDialog.h) and [ViewService.h (a33c4be)](https://github.com/fairybow/Fernanda/blob/38cf4be87281fbc12950a3cba979f6555d67cbca/Fernanda/src/ViewService.h))

In total, the settings pipeline spans roughly 300 lines of boilerplate across four files to do what a unified `settingChanged(key, value)` signal and a hash map could do in about 40.

A solid runner-up is this (related) unnecessary cast out of `QVariant` (repeated at least one other time in another panel):

```cpp
connect(
    wrap_mode_box,
    &QComboBox::currentIndexChanged,
    this,
    [&](int index) {
        emit settingChanged(
            Ini::Keys::EDITOR_WRAP_MODE,
            wrapMode_->control()
                ->itemData(index)
                .value<QTextOption::WrapMode>());
    });
```

---

# 0.99.0-beta.1 (Testing / Soft Release) - tag v0.99.0-beta.1

## Highlights

### Two Workspaces

- **Notepad**: operates directly on the filesystem. Open, edit, and save any plain text file on disk. Drag a file onto Fernanda or double-click to open it.
- **Notebook**: archive-based workspace where an entire project lives inside a single, portable `.fnx` file (standard 7zip). Organize files and folders with a virtual directory tree, drag-and-drop reorganization, and a soft-delete trash system. Multiple Notebooks can be open at once, and because `.fnx` is just 7zip, your content is always recoverable outside of Fernanda.

### Editor

Line numbers, current line highlight, selection handles, undo/redo, word wrap modes, overwrite mode, center-on-scroll, and configurable tab stops.

### Key Filters (Typing Enhancements)

Auto-close punctuation pairs, smart backspace, skip-closer, barge-past-closers on double-space or Enter, trailing punctuation gap closing, and smart quote/apostrophe handling. All individually toggleable.

### Tabs & Windows

Tabbed interface with drag-and-drop between windows (or drag a tab out to create a new one). Modified-file indicators, multiple windows per workspace, and full closure management with save prompts at every level.

### Themes

JSON-based editor themes with QSS template rendering. User themes supported (drop a [`.fernanda_editor`](https://github.com/fairybow/Fernanda/blob/main/Fernanda/resources/themes/Pocket.fernanda_editor) file into `~/.fernanda/themes/` and it's available immediately with hot reload).

### Settings

Non-modal settings dialog with live preview. Tiered settings: Notebooks inherit from the base configuration and can override per-project (stored inside the archive).

### Also Included

Word counter (line/word/character counts, selection-aware, cursor position), color bar (animated gradient feedback on save/startup), update checker via GitHub Releases, and magic-byte file type detection.

### Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested

## This Version's Dumbest Code Award :trophy:

When I first started working toward this project several years ago, I wrote code like this (this is real, I swear to God):

```cpp
if (checked)
{
    boolLineCount = true;
}
else
{
    boolLineCount = false;
};
```

With a semi-colon for good measure. It's amazing.

I've been at this for a while now, and though I think my code's improved a lot, I still find the occasional gems (sometimes something from last week). I wish I'd kept better track of older code from this last rewrite, as I'm sure it was full of questionable choices, but since I only thought about showcasing my worst code last minute, I had to settle for what was here now (or used to be).

Normally, I might save this section for dumb-yet-harmless code that's still in-place. Unfortunately, though, this one *had* to be removed.

So, without further ado, here's the erstwhile `wordCount_` from `WordCounter`:

```cpp
int wordCount_(const QString& text) const
{
    static QRegularExpression regex(LEADING_WS_REGEX_);
    auto words = text.split(regex, Qt::SkipEmptyParts);
    return words.count();
}
```

(See commented-out code at the bottom of [WordCounter.h (ee9a5c0)](https://github.com/fairybow/Fernanda/blob/8d59f04dd1bd05cc81d5756e5e548725b9a71d0d/Fernanda/src/WordCounter.h))

On a document the size of Moby Dick, the old word counter allocated and destroyed, *I think*, 215,831 individual heap objects on every keystroke to produce a single number. The new one uses an int and a bool.
