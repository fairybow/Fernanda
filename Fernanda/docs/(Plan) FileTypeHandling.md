# File Type Handling (Tentative Plan)

Tag: TODO FT

How Fernanda resolves, models, views, and persists different file types across its Workspace system.

See: [`MagicBytes.h`](../src/MagicBytes.h) (formerly `FileTypes.h`), [`FileService.h`](../src/FileService.h), [`AbstractFileModel.h`](../src/AbstractFileModel.h), [`AbstractFileView.h`](../src/AbstractFileView.h), [`Fnx.h`](../src/Fnx.h)

## Next Steps

- [x] `AbstractFileModel` rework: `data()` and `setData()` become pure virtual, `supportsModification()` becomes regular virtual defaulting to `false`. Remove `saveAs()` guard in FileService (Save As always works: every model must provide `data()`)
- [x] `FileTypes` header (central type registry, extensions, display names, dynamic filter builder). Existing `FileTypes` namespace renamed to `MagicBytes`
- [ ] Extension-first resolution in `FileService::newDiskFileModel_`
- [ ] Generalize FNX import to accept any file type and preserve source extension
- [ ] Explore whether FNX can avoid an `extension` attribute in Manifest.xml by querying the stored file directly (the file is already stored as `{uuid}.{ext}` in the archive)
- [ ] Tree view icons by file type
- [ ] Handle Notepad file renaming via TreeView
- [ ] Rename-triggered view/model re-evaluation (extension change -> new view)
- [ ] Replace hardcoded Tr filter strings with `FileTypes`-driven dynamic builder
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

`AbstractFileModel` uses pure virtuals for the fundamental data contract and a regular virtual for the opt-in editing capability:

| Method | Kind | Default | Meaning |
|--------|------|---------|---------|
| `data()` | Pure virtual |: | Returns raw bytes for persistence. Every model must implement this. |
| `setData()` | Pure virtual |: | Accepts raw bytes. Every model must implement this (each owns its storage strategy). |
| `supportsModification()` | Virtual | `false` | Whether content can be edited (undo/redo, dirty state). Used by `FileService::save()` and menu state. |

`data()` and `setData()` are pure virtual because no reasonable default storage exists at the base level: TextFileModel stores content in a QTextDocument, PdfFileModel holds raw bytes alongside a QBuffer/QPdfDocument, and each subclass has a different relationship to its data. Forcing each subclass to own its storage explicitly avoids ambiguous base-call conventions.

`supportsModification()` defaults to `false` because most types are read-only. Only TextFileModel (and future editable types like CorkboardFileModel) override to `true`.

**Save As has no guard.** Every model implements `data()`, so every model can be exported. `FileService::saveAs()` simply writes whatever `data()` returns. If a NoOp model returns empty bytes, an empty file is written: the user asked to Save As, so the system does it.

## File Types

### Special Types (Dedicated Handling)

These types are recognized by extension and receive their own model/view pairs or specialized views.

#### PDF

View-only document support. PDFs can be opened, viewed, and exported (Save As) but not edited.

| | |
|---|---|
| **Extension** | `.pdf` |
| **Model** | `PdfFileModel`: holds raw bytes (own `QByteArray`), exposes `QPdfDocument` via `QBuffer` |
| **View** | `PdfFileView`: wraps `QPdfView` (multi-page, fit-to-width) |
| **Modification** | No |
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
| **Notebook import** | Yes |
| **New file creation** | Yes (currently the only type that supports this) |

### NoOp (Transitional)

Currently serves as a catch-all for unrecognized file types. **This type will eventually be removed** as the fallback-to-PlainText strategy takes over. It may survive in a reduced role as the view shown when byte validation fails for a special type (e.g., a file named `.pdf` that isn't actually a PDF), or those cases may simply fall through to PlainText. See [Open Questions](#open-questions).

| | |
|---|---|
| **Model** | `NoOpFileModel`: implements `data()`/`setData()` with own `QByteArray` storage. Returns whatever was loaded (may be empty). |
| **View** | `NoOpFileView`: centered face glyph (`:')`) at 0.3 opacity) |
| **Modification** | No |

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

## FileTypes Registry

Extensions and type metadata needed across the application (themes need theme extensions, Application needs `.fnx`, Workspaces need `.pdf`, Tr needs type names for filter strings, etc.) should draw on a central registry.

The existing `FileTypes` namespace has been renamed to `MagicBytes` (byte-level signature detection). The `FileTypes` name is now free for a new header providing:
- A map of supported types to their metadata (display name, default extension, capabilities)
- A function to dynamically build file dialog filter strings from the registry (rather than hardcoding them in Tr)

The display name for each type needs to be translatable, so the filter builder would work with Tr rather than replacing it: Tr would delegate to the dynamic builder instead of owning hardcoded filter strings.

The relationship between `FileTypes` (registry) and `MagicBytes` (detection): they solve different problems. `MagicBytes` answers "what *is* this file?" `FileTypes` answers "what can Fernanda *do* with this file?" They remain separate; FileService bridges them.

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

## Implementation Steps

Work should happen on a **`file-types`** branch.

1. **This document**: Establish the plan and reference for all file type work.
2. (DONE) **`MagicBytes` rename**: Existing `FileTypes` namespace renamed to `MagicBytes`. Enum renamed from `HandledType` to `Kind`, default value from `PlainText` to `NoSignature`.
3. **`AbstractFileModel` rework**: `data()` and `setData()` become pure virtual (each subclass owns its storage). `supportsModification()` becomes regular virtual defaulting to `false`. Remove `saveAs()` guard in FileService.
4. **`FileTypes` header**: Central registry of supported types, extensions, display names, and capabilities. Dynamic filter string builder that Tr delegates to.
5. **Extension-first resolution in FileService**: Refactor `newDiskFileModel_` to check extension before bytes. Byte check becomes optional validation, not the routing mechanism.
6. **FNX all-file-type support**: Generalize `importTextFile` -> `importFile` to preserve source extension. Update stored filenames to `{uuid}.{ext}`. Update manifest handling.
7. **Tree view icons by type**: File-type-appropriate icons with a generic fallback for unrecognized types.
8. **Update Tr**: Replace hardcoded filter strings with calls to the `FileTypes` filter builder.

## Documents That Need Updating

Once this work lands, the following docs should be revised:

| Document | Changes needed |
|----------|----------------|
| **FileModelsAndViews.md** | Update `AbstractFileModel` API: `data()`/`setData()` now pure virtual, `supportsModification()` now regular virtual with `false` default. Add `PdfFileModel`/`PdfFileView` to concrete implementations. Update "Why Virtual Methods with Default No-Ops?" section. Note NoOp deprecation plan. |
| **Notebooks.md** | Document that FNX supports all file types, not just text. Update any `.txt`-only import references. Document `{uuid}.{ext}` file naming in archives. |
| **Architecture.md** | Mention `FileTypes` registry if it becomes a meaningful architectural element. Note `MagicBytes` rename. |
| **Roadmap.md** | Add `file-types` branch work items if not already tracked. |
