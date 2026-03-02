# File Type Handling (Tentative Plan)

How Fernanda resolves, models, views, and persists different file types across its Workspace system.

See: [`MagicBytes.h`](../src/MagicBytes.h), [`FileService.h`](../src/FileService.h), [`AbstractFileModel.h`](../src/AbstractFileModel.h), [`AbstractFileView.h`](../src/AbstractFileView.h), [`Fnx.h`](../src/Fnx.h)

## Next Steps

- [ ] Add `supportsWrite()` virtual to `AbstractFileModel` and update `FileService::saveAs()` guard
- [ ] Constants header (central type registry, extensions, display names, dynamic filter builder)
- [ ] Extension-first resolution in `FileService::newDiskFileModel_`
- [ ] Generalize FNX import to accept any file type and preserve source extension
- [ ] Handle Notepad file renaming via TreeView (currently only Notebook supports rename)
- [ ] Tree view icons by file type
- [ ] Replace hardcoded Tr filter strings with dynamic builder from Constants
- [ ] Rename-triggered view/model re-evaluation (extension change -> new view)
- [ ] Decide NoOp fate (keep for validation failures, or fall through to PlainText?)
- [ ] Update related docs (FileModelsAndViews, Notebooks, Architecture, Roadmap)

### Note on FNX extension attribute

The current plan stores extension in the XML manifest (`<file extension=".txt" />`). However, since archived files are stored as `{uuid}.{ext}`, we may be able to query the extension directly from the file in the `content/` directory rather than duplicating it in the manifest. Worth exploring: the attribute may be unnecessary if the file itself is the source of truth.

## Resolution Strategy

File type resolution follows an **extension-first, bytes-second** approach:

1. **Extension check**: Known special extensions (`.pdf`, `.fcb`, `.md`, `.fountain`, theme extensions) are matched first and route to their dedicated model/view pairs.
2. **Byte validation** (optional): For types where it matters (e.g., confirming a `.pdf` is actually a PDF via magic bytes), a secondary check can validate after extension matching.
3. **Fallback to PlainText**: Anything that doesn't match a known special extension is treated as plain text.
4. **Validation failure**: If a file matches a special extension but fails byte validation (e.g., a `.txt` file renamed to `.pdf`), it either falls through to PlainText or displays as NoOp. See [Open Questions](#open-questions).

### FNX Archives

FNX files (`.fnx`) are a special case handled at the Application level, not by FileService's type resolution. Application detects FNX files and routes them to Notebook Workspaces. Within a Notebook, individual files are resolved by the extension stored in the XML manifest (and present in the stored filename itself).

### Rename-Triggered Resolution

When a file is renamed and its extension changes, the system should re-evaluate which model/view pair is appropriate. For example, renaming `notes.txt` to `notes.md` should swap from a plain text view to a Markdown view. This implies FileService (or the Workspace layer) needs to detect extension changes on rename and potentially reconstruct the view, and possibly the model if the new type requires a different model class.

## Model Capabilities

`AbstractFileModel` exposes three capability levels:

| Virtual | Meaning | Used by |
|---------|---------|---------|
| `supportsModification()` | Content can be edited (undo/redo, dirty state) | `FileService::save()`, menu state |
| `supportsWrite()` | Data can be written to disk (Save As) | `FileService::saveAs()` |
| `data()` | Raw bytes for persistence | Both save paths |

`supportsWrite()` defaults to `supportsModification()`, so only read-only-but-exportable types like PDF need to override it.

## File Types

### Special Types (Dedicated Handling)

These types are recognized by extension and receive their own model/view pairs or specialized views.

#### PDF

View-only document support. PDFs can be opened, viewed, and exported (Save As) but not edited.

| | |
|---|---|
| **Extension** | `.pdf` |
| **Model** | `PdfFileModel`: holds raw bytes, exposes `QPdfDocument` |
| **View** | `PdfFileView`: wraps `QPdfView` (multi-page, fit-to-width) |
| **Modification** | No |
| **Write** | Yes (Save As exports the original bytes) |
| **Notebook import** | Yes |
| **New file creation** | No |
| **Byte validation** | Yes (PDF magic bytes) |

#### Corkboard (Tentative)

A visual planning tool for organizing story elements. Corkboard files are JSON stored with a special extension. Multiple corkboard files can exist per project, and they can be saved to disk via Notepad like any other file.

| | |
|---|---|
| **Extension** | `.fcb` (Fernanda Corkboard) |
| **Model** | `CorkboardFileModel`: JSON data |
| **View** | `CorkboardFileView`: interactive board of movable index cards |
| **Modification** | Yes |
| **Write** | Yes |
| **Notebook import** | Yes |
| **New file creation** | Yes |

Cards can be linked to existing text files (within the same Notebook or on disk). Linked file tracking needs to handle moves and deletions: likely storing the linked file's display name as a breadcrumb and showing a "missing" state if the target can't be resolved.

#### Theme Editor (Long-Term)

Custom views for Fernanda's own window and editor theme files. These are two distinct file types (Fernanda Window theme and Fernanda Editor theme) that share a similar editing approach. The underlying data is JSON.

This is **way down the road**. The initial concept is not necessarily a fully custom view: it may be a text view augmented with properties that show color pickers beside existing value fields, where the user can still type directly and pickers pop up contextually. The view would draw on a theme API exposed from `Themes.h`.

| | |
|---|---|
| **Extensions** | Fernanda window theme ext, Fernanda editor theme ext (TBD) |
| **Model** | Possibly `TextFileModel` or a thin subclass |
| **View** | Augmented text view with color pickers / form fields |
| **Modification** | Yes |
| **Write** | Yes |

#### Markdown (Future)

Markdown files would receive a dedicated view (likely a rendered preview or a split edit/preview). The model may be `TextFileModel` or a subclass: the underlying data is still plain text; the distinction is in how it's displayed.

| | |
|---|---|
| **Extension** | `.md` |
| **Model** | `TextFileModel` or subclass |
| **View** | Markdown-aware view (rendered preview, split mode, etc.) |

#### Fountain (Future)

Fountain (screenwriting format) files would follow the same pattern as Markdown: plain text data with a specialized view for screenplay formatting.

| | |
|---|---|
| **Extension** | `.fountain` |
| **Model** | `TextFileModel` or subclass |
| **View** | Fountain-aware view (screenplay formatting) |

#### Diff (Stretch)

A diff view for comparing file versions. Details TBD.

### Plain Text (Universal Fallback)

The default and primary file type. Everything that isn't a recognized special type opens as plain text. This includes files with no extension, unknown extensions, or recognized extensions that fail byte validation (depending on the resolution chosen: see [Open Questions](#open-questions)).

| | |
|---|---|
| **Extensions** | `.txt` (default for new files), any unrecognized extension |
| **Model** | `TextFileModel`: wraps `QTextDocument` |
| **View** | `TextFileView`: wraps `PlainTextEdit` |
| **Modification** | Yes |
| **Write** | Yes |
| **Notebook import** | Yes |
| **New file creation** | Yes (currently the only type that supports this) |

### NoOp (Transitional)

Currently serves as a catch-all for unrecognized file types. **This type will eventually be removed** as the fallback-to-PlainText strategy takes over. It may survive in a reduced role as the view shown when byte validation fails for a special type (e.g., a file named `.pdf` that isn't actually a PDF), or those cases may simply fall through to PlainText. See [Open Questions](#open-questions).

| | |
|---|---|
| **Model** | `NoOpFileModel`: returns empty data |
| **View** | `NoOpFileView`: centered face glyph (`:'`) at 0.3 opacity) |
| **Modification** | No |
| **Write** | No |

## FNX Manifest and File Storage

Within a Notebook's 7zip archive, every file is stored in the `content/` directory named by UUID **with its real extension** (e.g., `content/{uuid}.txt`, `content/{uuid}.pdf`). Files are not stored as bare UUIDs.

The XML manifest tracks each file's metadata:

```xml
<file name="Chapter One" uuid="abc-123" extension=".txt" />
<file name="Reference" uuid="def-456" extension=".pdf" />
```

FNX accepts import of **any file type**, mirroring Notepad's open-anything philosophy.

### Extension handling

- **Import**: The extension is taken from the source file on disk (including files with no extension or custom extensions). A PDF retains `.pdf`; a file called `notes` with no extension retains no extension.
- **New file creation**: The default extension comes from the file type's definition (currently only `.txt`).
- **Type resolution on open**: When a Notebook element is opened, the extension from the manifest determines which model/view pair FileService creates. The manifest is authoritative within the archive.
- **Tree view icons**: File type should determine the icon shown in the tree view, with a generic icon for unrecognized types.

## Constants

Extensions and type metadata needed across the application (themes need theme extensions, Application needs `.fnx`, Workspaces need `.pdf`, Tr needs type names for filter strings, etc.) should draw on a central registry.

This could be a `Constants` header providing:
- A map of supported types to their metadata (display name, default extension, capabilities)
- A function to dynamically build file dialog filter strings from the registry (rather than hardcoding them in Tr)
- Shared presentation constants (e.g., opacity values like 0.3 for separators and NoOp face)

The display name for each type needs to be translatable, so the filter builder would work with Tr rather than replacing it: Tr would delegate to the dynamic builder instead of owning hardcoded filter strings.

The relationship between Constants and `MagicBytes::KnownType` (file signatures): they solve different problems. `KnownType` is detection ("what *is* this file?"), Constants is application policy ("what can Fernanda *do* with this file?"). They remain separate; FileService bridges them.

## FileService Resolution Flow

```
openFilePathIn(window, path)
    |
    v
Extension matches known special type?
    |-- .pdf ----> (byte check) ----> PdfFileModel + PdfFileView
    |-- .fcb ----> CorkboardFileModel + CorkboardFileView
    |-- .md  ----> TextFileModel + MarkdownView
    |-- etc.
    |
    |   (byte check failed?)
    |   |-- fall through to PlainText? or NoOp? (see Open Questions)
    |
    v
Fallback: TextFileModel + TextFileView
```

## Open Questions

- **Byte validation failure**: When a file has a special extension but fails byte validation (e.g., `fake.pdf` that isn't a real PDF), should it fall through to PlainText or show NoOp? PlainText is more useful (user can see the raw content), NoOp is more honest (the file isn't what it claims). This also affects whether NoOp survives at all.
- **Rename view swap mechanics**: What's the right layer for detecting extension changes and triggering model/view reconstruction? Does the Workspace handle this, or FileService?
- **Constants scope**: How much belongs in Constants vs. staying local? Opacity values are presentation; type metadata is structural. Maybe these are two different things that happen to share a header.

## Implementation Steps

Work should happen on a **`file-types`** branch.

1. **This document**: Establish the plan and reference for all file type work.
2. **`supportsWrite()` on AbstractFileModel**: Add the virtual with default returning `supportsModification()`. Override in `PdfFileModel` to return `true`. Update `FileService::saveAs()` guard to use `supportsWrite()` instead of `supportsModification()`.
3. **Constants header**: Central registry of supported types, extensions, display names, and capabilities. Dynamic filter string builder that Tr delegates to.
4. **Extension-first resolution in FileService**: Refactor `newDiskFileModel_` to check extension before bytes. Byte check becomes optional validation, not the routing mechanism.
5. **FNX all-file-type support**: Generalize `importTextFile` -> `importFile` to preserve source extension. Update stored filenames to `{uuid}.{ext}`. Update manifest handling.
6. **Tree view icons by type**: File-type-appropriate icons with a generic fallback for unrecognized types.
7. **Update Tr**: Replace hardcoded filter strings with calls to the Constants filter builder.

## Documents That Need Updating

Once this work lands, the following docs should be revised:

| Document | Changes needed |
|----------|----------------|
| **FileModelsAndViews.md** | Add `supportsWrite()` to capability table and API listing. Add `PdfFileModel`/`PdfFileView` to concrete implementations. Update "Why Virtual Methods with Default No-Ops?" section. Note NoOp deprecation plan. |
| **Notebooks.md** | Document that FNX supports all file types, not just text. Update any `.txt`-only import references. Document `{uuid}.{ext}` file naming in archives. |
| **Architecture.md** | Mention Constants if it becomes a meaningful architectural element (central type registry). |
| **Roadmap.md** | Add `file-types` branch work items if not already tracked. |
