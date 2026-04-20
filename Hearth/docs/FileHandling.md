# File Handling

TODO: Make sure this is consistent with code (and reduce direct code references)

How Hearth identifies, opens, and saves files.

See: [`FileService.h`](../src/services/FileService.h), [`Files.h`](../src/core/Files.h), [`MagicBytes.h`](../src/core/MagicBytes.h), [`AbstractFileModel.h`](../src/models/AbstractFileModel.h), and [`Nbx.h`](../src/nbx/Nbx.h)

## Overview

Hearth aims to open any file correctly regardless of its name. A PDF with the wrong extension still opens as a PDF. A text file with no extension opens as text. The user's choice of filename is always respected.

File handling has two distinct concerns:

- **Identification**: Determining what type of file something is
- **Policy**: Deciding what to do with it (which model/view to use, whether to allow editing, etc.)

`MagicBytes` handles identification. `Files` and `FileService` handle policy.

## How Files Are Identified

When a file is opened, Hearth uses a two-tier identification strategy.

### Tier 1: Content Signatures (Magic Bytes)

Binary formats (PDF, and eventually images, etc.) embed recognizable byte patterns at the start of their data. Hearth checks these first. If a file's bytes match a known binary format, it is identified by its content regardless of what extension the file has. A PDF named `chapter-notes.xyz` is still a PDF.

This tier is authoritative for any format that has a signature. Extension is irrelevant.

### Tier 2: Extension

Text-based formats have no distinguishing byte signature. A Markdown file, a Fountain screenplay, and a plain `.txt` file are all just text at the byte level. For these formats, the file's extension is the only available signal. Hearth checks the extension against its registry of known text-based types and routes accordingly.

### Fallthrough

If neither tier produces a match (no recognized signature and no special extension), the file opens as plain text. This is also the fallthrough for binary signatures that Hearth recognizes but does not yet have a dedicated viewer for.

```
Open file
  |
  v
Known byte signature?
  |-- Yes -> open with dedicated handler (e.g., PDF viewer)
  |-- No  -> known text extension? (.md, .fountain, etc.)
  |             |-- Yes -> open with dedicated text handler
  |             |-- No  -> open as plain text
```

## Files Registry

Extensions and type metadata needed across the application are centralized in the `Files` namespace. It provides a constexpr table of supported types mapped to their canonical extensions (with aliases like `.jpg` for `.jpeg`), `canonicalExt(Type)` to retrieve the canonical extension for a type, and `fromPath(path)` to resolve a file's extension to a Type (Plain text for anything unrecognized).

`FileTypes` and `MagicBytes` solve different problems and remain separate. `MagicBytes` answers "what is this file?" by inspecting bytes. `FileTypes` answers "what can Hearth do with this file?" by mapping extensions to known types. `FileService` bridges them via the two-tier resolution.

## File Types

### PDF

View-only document support. PDFs can be opened, viewed, and exported (Save As) but not edited. Detected by magic bytes (Tier 1), so the file extension does not matter.

| | |
|---|---|
| **Extension** | `.pdf` (canonical, but detection is by bytes) |
| **Model** | `PdfFileModel`: holds raw bytes (own `QByteArray`), exposes `QPdfDocument` via `QBuffer` |
| **View** | `PdfFileView`: wraps `QPdfView` (multi-page, fit-to-width) |
| **Modification** | No |
| **Notebook import** | Yes |
| **New file creation** | No |
| **Detection** | Tier 1 (magic bytes) |

### Corkboard (Tentative)

A visual planning tool for organizing story elements. Corkboard files are JSON stored with a special extension. Multiple corkboard files can exist per project, and they can be saved to disk via Notepad like any other file. Detected by extension (Tier 2) since the underlying data is plain text JSON.

| | |
|---|---|
| **Extension** | `.hcb` (Hearth Corkboard) |
| **Model** | `CorkboardFileModel`: JSON data |
| **View** | `CorkboardFileView`: interactive board of movable index cards |
| **Modification** | Yes |
| **Notebook import** | Yes |
| **New file creation** | Yes |
| **Detection** | Tier 2 (extension) |

Cards can be linked to existing text files (within the same Notebook or on disk). Linked file tracking needs to handle moves and deletions: likely storing the linked file's display name as a breadcrumb and showing a "missing" state if the target can't be resolved.

### Theme Editor (Long-Term)

Custom views for Hearth's own window and editor theme files. These are two distinct file types (Hearth Window theme and Hearth Editor theme) that share a similar editing approach. The underlying data is JSON. Detected by extension (Tier 2).

This is way down the road. The initial concept is not necessarily a fully custom view: it may be a text view augmented with properties that show color pickers beside existing value fields, where the user can still type directly and pickers pop up contextually. The view would draw on a theme API exposed from `Themes.h`.

| | |
|---|---|
| **Extensions** | Hearth window theme ext, Hearth editor theme ext (TBD) |
| **Model** | Possibly `TextFileModel` or a thin subclass |
| **View** | Augmented text view with color pickers / form fields |
| **Modification** | Yes |
| **Detection** | Tier 2 (extension) |

### Markdown (Future)

Markdown files would receive a dedicated view (likely a rendered preview or a split edit/preview). The model may be `TextFileModel` or a subclass: the underlying data is still plain text; the distinction is in how it is displayed. Detected by extension (Tier 2).

| | |
|---|---|
| **Extension** | `.md` |
| **Model** | `TextFileModel` or subclass |
| **View** | Markdown-aware view (rendered preview, split mode, etc.) |
| **Detection** | Tier 2 (extension) |

### Fountain (Future)

Fountain (screenwriting format) files would follow the same pattern as Markdown: plain text data with a specialized view for screenplay formatting. Detected by extension (Tier 2).

| | |
|---|---|
| **Extension** | `.fountain` |
| **Model** | `TextFileModel` or subclass |
| **View** | Fountain-aware view (screenplay formatting) |
| **Detection** | Tier 2 (extension) |

### Diff (Stretch)

A diff view for comparing file versions. Details TBD.

### Plain Text (Universal Fallback)

The default and primary file type. Everything that falls through both tiers opens as plain text. This includes: files with no extension, files with unrecognized extensions, files with known signatures but no handler yet (e.g., PNG, GIF), and files with no known signature and no special extension.

| | |
|---|---|
| **Extensions** | `.txt` (default for new files), any unrecognized extension |
| **Model** | `TextFileModel`: wraps `QTextDocument` |
| **View** | `TextFileView`: wraps `PlainTextEdit` |
| **Modification** | Yes |
| **Notebook import** | Yes |
| **New file creation** | Yes (currently the only type that supports this) |

### NoOp (Not Currently Used)

Previously served as a catch-all for unrecognized file types. Currently not used in the resolution flow since everything falls through to PlainText. Kept around for potential future use, e.g., for large unsupported binary files (images, etc.) where opening as text would be wasteful. May also be useful as a distinct "this file type will be supported eventually" placeholder.

| | |
|---|---|
| **Model** | `NoOpFileModel`: implements `data()`/`setData()` with own `QByteArray` storage. Returns whatever was loaded (may be empty). |
| **View** | `NoOpFileView`: centered face glyph (`:')`) at 0.3 opacity) |
| **Modification** | No |

## Extensions and User Choice

Hearth does not force or auto-append file extensions. When saving, the suggested filename includes an appropriate extension (drawn from `FileMeta::preferredExt()`, which uses the file's existing path extension if on disk or `Files::canonicalExt(type)` if off-disk), but if the user changes or removes it, that choice is honored.

This means a file's extension can affect how it is handled:

- Renaming `notes.md` to `notes.txt` means Hearth will treat it as plain text instead of Markdown the next time it is opened. The content is unchanged, only the handling differs.
- Removing a file's extension entirely causes it to fall through to plain text.
- Changing the extension of a binary file (e.g., renaming a PDF to `.txt`) has no effect on identification, since Tier 1 (byte signature) takes priority over extension.

This behavior is intentional and consistent with other editors. The extension is the user's stated intent for how the file should be treated.

In the future, renaming a file's extension while it is open will trigger a view re-evaluation, swapping to the appropriate handler without needing to close and reopen the file. This only applies to Tier 2 types (text-based), since Tier 1 types are identified by content regardless of name.

## NBX Archives

NBX files (`.hearthx`) are Hearth Notebook archives. They are the only file type handled outside the two-tier identification system. `Application` and Workspaces intercept NBX files before they ever reach `FileService`.

### How NBX files are detected

`Nbx::Io::isNbxFile` checks two things: the file must have the `.hearthx` extension *and* must be a valid ZIP archive (verified by magic bytes). Both checks must pass.

### How NBX files enter the system

Several paths through the application encounter files that might be NBX archives:

- **Command-line arguments** (including drag-onto-exe and relaunch): `Application::parseArgs_` runs `isNbxFile` on each argument. Passing files are routed to a Notebook Workspace. Failing files (including corrupt or renamed NBX archives) are routed to Notepad as regular files.
- **Notepad "Open file" dialog**: After the user selects files, each is checked with `isNbxFile`. Passing files emit `openNotebookRequested`. Failing files are opened normally via `FileService` (two-tier identification).
- **Notepad TreeView double-click**: Same `isNbxFile` check and routing as the Open dialog.
- **"Open Notebook" menu** (available in all Workspaces): The file dialog is filtered to `.hearthx` files. After selection, `isNbxFile` validates the file. If the check fails, the file is silently not opened. This is the only path that refuses rather than falling through.

### What happens when NBX detection fails

In most code paths, a file that looks like it might be an NBX archive but fails `isNbxFile` is simply opened as a regular file. This means the user may see binary content (the raw ZIP data rendered as text). This is consistent with the general principle that Hearth opens anything: the file is not an NBX archive as far as the system is concerned, so it is treated like any other file.

The one exception is the "Open Notebook" menu dialog, which is specifically for opening Notebooks and silently refuses invalid files.

### What happens when an NBX extension is changed

If an NBX file is renamed (e.g., `MyProject.hearthx` to `MyProject.zip`, even though it IS a ZIP), `isNbxFile` will fail because the extension check fails. The file will not be recognized as a Notebook from any path. It will be opened as a regular file by Notepad, showing binary content.

### NBX files inside NBX archives

Individual files within an NBX archive go through the same two-tier identification as any other file when opened. A user can import an NBX archive into another Notebook (import accepts any file type). If opened from within the Notebook, it goes through `FileService` with no `isNbxFile` intercept. MagicBytes detects the ZIP signature, but since there is no dedicated handler for ZIP archives in `FileService`, it falls through to plain text. The user sees binary content. This is expected and not a supported workflow.

Opening an inner NBX as a functional Notebook was considered and deliberately deferred. The implementation would require nested archive lifecycle management (extraction, save propagation, closure coordination), which conflicts with the current architecture where Workspaces are independent peers. A simpler "open as independent Notebook" approach was also considered, but it creates a confusing UX: edits to the inner Notebook would not propagate back to the outer archive, contradicting user expectations.

### How files are stored in NBX archives

Files inside a Notebook ZIP archive live in a `content/` directory, named by UUID with their real extension (e.g., `content/{uuid}.txt`, `content/{uuid}.pdf`). The XML manifest tracks metadata for each file:

```xml
<file name="Chapter One" uuid="abc-123" extension=".txt" />
<file name="Reference" uuid="def-456" extension=".pdf" />
```

The `extension` attribute is the index that `Fnx::Xml::relPath()` uses to reconstruct the content path (`uuid + ext`). It is populated from reality: `fsPath.extQString()` on import, `FileTypes::canonicalExt(type)` for new files. The `name` attribute is the user-facing display name shown in the tree view.

For files with compound extensions (e.g., `archive.tar.gz`), `std::filesystem::path::extension()` returns only the final extension (`.gz`), and the stem (`archive.tar`) becomes the display name. The full original filename can be reconstructed on export by joining `name + ext`.

## File Operations

All file operations that involve user interaction (dialogs, prompts) are listed below, organized by where they occur. Operations marked with a filter use `QFileDialog` and may constrain the visible or selectable file types.

### Application

| Operation | Description | Filter |
|---|---|---|
| **Args / drag-to-open / relaunch** | Files passed via command line or drag-onto-exe. `isNbxFile` partitions into Notebook files and regular files. Regular files go to Notepad. | None (filesystem) |

### All Workspaces

These operations appear in the menu bar of every Workspace (Notepad and all Notebooks).

| Operation | Description | Filter |
|---|---|---|
| **New Notebook** | Prompts for a name, creates a new `.hearthx` path, and emits `newNotebookRequested`. No file dialog. | None (name prompt only) |
| **Open Notebook** | File dialog for selecting a `.hearthx` file. Validates with `isNbxFile` after selection. Silently refuses if invalid. | `*.hearthx` |

### Notepad

| Operation | Description | Filter |
|---|---|---|
| **New Tab** | Creates an off-disk text file (no dialog). Will eventually expand to offer other creatable types (Markdown, Corkboard, etc.) via an overflow menu or right-click on the new tab button. | None |
| **Open File** | File dialog for selecting files. Each file is checked with `isNbxFile`; passing files go to a Notebook, others open via `FileService` (two-tier). | All files |
| **TreeView double-click** | Same `isNbxFile` routing as Open File. | None (filesystem) |
| **TreeView rename** | Inline rename via selected-click or F2. Renames the file on disk via `QFileSystemModel`. If the file has an open model, its path is updated via `FileMeta::setPath`, which cascades through FileService's path hash and updates tab titles. Directory rename is disabled (stripped from `flags()` via `NotepadFileSystemModel_`). Renaming an open Notebook's `.hearthx` file is not currently prevented and can cause the Notebook's save target to become stale. | None (filesystem) |
| **Save** | Writes modified content to the file's existing path. Only operates on modifiable models with changes. | None |
| **Save As** | File dialog for choosing a new path. Writes the model's data to that path. No extension is forced. The suggested filename comes from `FileMeta`, which provides the appropriate extension. | All files |
| **Save All in Window** | Saves all modified models in the current window. Prompts Save As for any that are not yet on disk. | Per-file as needed |
| **Save All** | Saves all modified models across all Notepad windows. Prompts Save As for any that are not yet on disk. | Per-file as needed |

### Notebook

| Operation | Description | Filter |
|---|---|---|
| **New File** | Creates a new file inside the archive via `Fnx::Xml::addNewFile(type)` (no dialog). Currently only creates plain text (`FileTypes::PlainText`). Will eventually expand to other creatable types, matching Notepad's future expansion. | None |
| **New Folder** | Creates a new virtual folder in the archive's XML manifest. No file is created. | None |
| **Import Files** | File dialog for selecting files from disk. Accepts any file type (no filter). Selected files are copied into the archive's `content/` directory as `{uuid}.{ext}` (extension taken from source path via `fsPath.extQString()`). The source file's stem becomes the display name in the manifest. Imported files are opened after import. | All files |
| **Export File** | Save As dialog for exporting a single file from the archive to disk. Available from the tree view context menu for file elements only (`FnxModel::isFile`). The suggested filename is reconstructed from `name + ext`. Copies the file from the working directory to the chosen destination. | All files |
| **TreeView double-click** | Opens the selected file from the archive via `FileService` (two-tier). No `isNbxFile` check. | None |
| **Save** | Saves the Notebook archive. Prompts Save As if the archive is not yet on disk. Also saves all modified file models within the archive. | None (or `*.hearthx` if prompting) |
| **Save As** | File dialog for saving the Notebook archive to a new `.hearthx` path. | `*.hearthx` |