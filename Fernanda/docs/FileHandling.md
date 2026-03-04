# File Handling

How Fernanda identifies, opens, and saves files.

See: [`FileService.h`](../src/FileService.h), [`FileTypes.h`](../src/FileTypes.h), [`MagicBytes.h`](../src/MagicBytes.h), [`AbstractFileModel.h`](../src/AbstractFileModel.h), [`Fnx.h`](../src/Fnx.h)

## Overview

Fernanda aims to open any file correctly regardless of its name. A PDF with the wrong extension still opens as a PDF. A text file with no extension opens as text. The user's choice of filename is always respected.

File handling has two distinct concerns:

- **Identification**: Determining what kind of file something is
- **Policy**: Deciding what to do with it (which model/view to use, whether to allow editing, etc.)

`MagicBytes` handles identification. `FileTypes` and `FileService` handle policy.

## How Files Are Identified

When a file is opened, Fernanda uses a two-tier identification strategy.

### Tier 1: Content Signatures (Magic Bytes)

Binary formats (PDF, and eventually images, etc.) embed recognizable byte patterns at the start of their data. Fernanda checks these first. If a file's bytes match a known binary format, it is identified by its content regardless of what extension the file has. A PDF named `chapter-notes.xyz` is still a PDF.

This tier is authoritative for any format that has a signature. Extension is irrelevant.

### Tier 2: Extension

Text-based formats have no distinguishing byte signature. A Markdown file, a Fountain screenplay, and a plain `.txt` file are all just text at the byte level. For these formats, the file's extension is the only available signal. Fernanda checks the extension against its registry of known text-based types and routes accordingly.

### Fallthrough

If neither tier produces a match (no recognized signature and no special extension), the file opens as plain text. This is also the fallthrough for binary signatures that Fernanda recognizes but does not yet have a dedicated viewer for.

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

## Extensions and User Choice

Fernanda does not force or auto-append file extensions. When saving, the suggested filename includes an appropriate extension, but if the user changes or removes it, that choice is honored.

This means a file's extension can affect how it is handled:

- Renaming `notes.md` to `notes.txt` means Fernanda will treat it as plain text instead of Markdown the next time it is opened. The content is unchanged, only the handling differs.
- Removing a file's extension entirely causes it to fall through to plain text.
- Changing the extension of a binary file (e.g., renaming a PDF to `.txt`) has no effect on identification, since Tier 1 (byte signature) takes priority over extension.

This behavior is intentional and consistent with other editors. The extension is the user's stated intent for how the file should be treated.

In the future, renaming a file's extension while it is open will trigger a view re-evaluation, swapping to the appropriate handler without needing to close and reopen the file. This only applies to Tier 2 types (text-based), since Tier 1 types are identified by content regardless of name.

## FNX Archives

FNX files (`.fnx`) are Fernanda Notebook archives. They are the only file type handled outside the two-tier identification system. `Application` and Workspaces intercept FNX files before they ever reach `FileService`.

### How FNX files are detected

`Fnx::Io::isFnxFile` checks two things: the file must have the `.fnx` extension *and* must be a valid 7zip archive (verified by magic bytes). Both checks must pass.

### How FNX files enter the system

Several paths through the application encounter files that might be FNX archives:

- **Command-line arguments** (including drag-onto-exe and relaunch): `Application::parseArgs_` runs `isFnxFile` on each argument. Passing files are routed to a Notebook Workspace. Failing files (including corrupt or renamed FNX archives) are routed to Notepad as regular files.
- **Notepad "Open file" dialog**: After the user selects files, each is checked with `isFnxFile`. Passing files emit `openNotebookRequested`. Failing files are opened normally via `FileService` (two-tier identification).
- **Notepad TreeView double-click**: Same `isFnxFile` check and routing as the Open dialog.
- **"Open Notebook" menu** (available in all Workspaces): The file dialog is filtered to `.fnx` files. After selection, `isFnxFile` validates the file. If the check fails, the file is silently not opened. This is the only path that refuses rather than falling through.

### What happens when FNX detection fails

In most paths, a file that looks like it might be an FNX archive but fails `isFnxFile` is simply opened as a regular file. This means the user may see binary content (the raw 7zip data rendered as text). This is consistent with the general principle that Fernanda opens anything: the file is not an FNX archive as far as the system is concerned, so it is treated like any other file.

The one exception is the "Open Notebook" menu dialog, which is specifically for opening Notebooks and silently refuses invalid files.

### What happens when an FNX extension is changed

If an FNX file is renamed (e.g., `MyProject.fnx` to `MyProject.zip`), `isFnxFile` will fail because the extension check fails. The file will not be recognized as a Notebook from any path. It will be opened as a regular file by Notepad, showing binary content.

### FNX files inside FNX archives

Individual files within an FNX archive go through the same two-tier identification as any other file when opened. It is possible (once import is generalized to accept any file type) for a user to import an FNX archive into another Notebook. If opened from within the Notebook, it will go through `FileService` with no `isFnxFile` intercept. MagicBytes will detect the 7zip signature, but since there is no dedicated handler for 7zip in `FileService`, it will fall through to plain text. The user will see binary content. This is expected and not a supported workflow.

## File Operations

All file operations that involve user interaction (dialogs, prompts) are listed below, organized by where they occur. Operations marked with a filter use `QFileDialog` and may constrain the visible or selectable file types.

### Application

| Operation | Description | Filter |
|-----------|-------------|--------|
| **Args / drag-to-open / relaunch** | Files passed via command line or drag-onto-exe. `isFnxFile` partitions into Notebook files and regular files. Regular files go to Notepad. | None (filesystem) |

### All Workspaces

These operations appear in the menu bar of every Workspace (Notepad and all Notebooks).

| Operation | Description | Filter |
|-----------|-------------|--------|
| **New Notebook** | Prompts for a name, creates a new `.fnx` path, and emits `newNotebookRequested`. No file dialog. | None (name prompt only) |
| **Open Notebook** | File dialog for selecting a `.fnx` file. Validates with `isFnxFile` after selection. Silently refuses if invalid. | `*.fnx` |

### Notepad

| Operation | Description | Filter |
|-----------|-------------|--------|
| **New Tab** | Creates an off-disk text file (no dialog). Will eventually expand to offer other creatable types (Markdown, Corkboard, etc.) via an overflow menu or right-click on the new tab button. | None |
| **Open File** | File dialog for selecting files. Each file is checked with `isFnxFile`; passing files go to a Notebook, others open via `FileService` (two-tier). | All files |
| **TreeView double-click** | Same `isFnxFile` routing as Open File. | None (filesystem) |
| **Save** | Writes modified content to the file's existing path. Only operates on modifiable models with changes. | None |
| **Save As** | File dialog for choosing a new path. Writes the model's data to that path. No extension is forced. | All files |
| **Save All in Window** | Saves all modified models in the current window. Prompts Save As for any that are not yet on disk. | Per-file as needed |
| **Save All** | Saves all modified models across all Notepad windows. Prompts Save As for any that are not yet on disk. | Per-file as needed |

### Notebook

| Operation | Description | Filter |
|-----------|-------------|--------|
| **New File** | Creates a new text file inside the archive (no dialog). Will eventually expand to other creatable types, matching Notepad's future expansion. | None |
| **New Folder** | Creates a new virtual folder in the archive's XML manifest. No file is created. | None |
| **Import Files** | File dialog for selecting files from disk. Selected files are copied into the archive and opened. Currently filtered; will be generalized to accept any file type. | All supported (will become all files) (NB: Imported files will have their filenames replaced with a UUID and the filename will become the metadata title) |
| **TreeView double-click** | Opens the selected file from the archive via `FileService` (two-tier). No `isFnxFile` check. | None |
| **Save** | Saves the Notebook archive. Prompts Save As if the archive is not yet on disk. Also saves all modified file models within the archive. | None (or `*.fnx` if prompting) |
| **Save As** | File dialog for saving the Notebook archive to a new `.fnx` path. | `*.fnx` |
| **Export** (planned) | Save a file from within the archive to a location on disk. | TBD |
