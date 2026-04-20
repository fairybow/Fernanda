# File Imports

How files are imported into Notepad and Notebook workspaces.

See: [`NotepadImport.h`](../src/workspaces/NotepadImport.h), [`NotebookImport.h`](../src/workspaces/NotebookImport.h), [`Files.h`](../src/core/Files.h), [`Workspace.cpp`](../src/workspaces/Workspace.cpp), [`Notepad.h`](../src/workspaces/Notepad.h), and [`Notebook.h`](../src/workspaces/Notebook.h).

## Overview

All workspaces share a single "Import..." action in the File menu. The action calls two virtual methods: `importFilter()` (what the file dialog shows) and `importFiles()` (what happens with the selected files). Workspace subclasses override both.

The two workspaces have fundamentally different import goals:

| Workspace | Goal | Accepted files | Output |
|---|---|---|---|
| Notepad | Convert to plain text and open in new tab(s) | Conversion-only (DOCX and RTF) | Off-disk `TextFileModel` with converted content |
| Notebook | Add file(s) to the archive and open in new tab(s) | Any file (converts DOCX and RTF) | New content entry in the NBX archive |

## Notepad

Notepad imports are conversion-only. The file dialog filters to supported conversion formats (currently DOCX and RTF). Each selected file is run through `NotepadImport::process`, which checks the file with compound identification (extension + magic bytes), converts it to plain text, and returns the result. The converted text is opened in a new tab via `FileService::openOffDiskPlainTextFileIn`, using the source file's stem as the tab title.

Files that do not match a known conversion type produce an empty result and are skipped. The rolling open directory is not updated for imports.

## Notebook

Notebook imports accept any file. The file dialog offers "All Files" and a grouped conversion filter. Each selected file is run through `NotebookImport::process`, which handles two cases:

**Convertible files** (DOCX, RTF): Identified by compound checks (extension + magic bytes), converted to plain text. The result carries `Files::PlainText` as its type and `.txt` as its extension.

**Passthrough files** (everything else): Read as raw bytes. The file's type is resolved via two-tier identification (magic bytes first, then extension). The original extension is preserved.

Each result is added to the archive via `FnxModel::addNewFile`, which delegates to `Fnx::Xml::addNewFile`. The extension parameter flows into the XML manifest's `extension` attribute and determines the on-disk content filename (`{uuid}{ext}`). The source file's stem becomes the display name. Imported files are opened after insertion, and the tree view expands to show the last imported file.