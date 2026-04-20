# File Type Handling (Tentative Plan)

Tag: TODO FT

How Hearth resolves, models, views, and persists different file types across its Workspace system.

See: [`MagicBytes.h`](../src/core/MagicBytes.h) (formerly `FileTypes.h`), [`FileService.h`](../src/services/FileService.h), [`AbstractFileModel.h`](../src/models/AbstractFileModel.h), [`AbstractFileView.h`](../src/views/AbstractFileView.h), [`Nbx.h`](../src/nbx/Nbx.h)

## Next Steps

- [x] `AbstractFileModel` rework: `data()` and `setData()` become pure virtual, `supportsModification()` becomes regular virtual defaulting to `false`. Remove `saveAs()` guard in FileService (Save As always works: every model must provide `data()`)
- [x] `FileTypes` header (central type registry, canonical extensions). Existing `FileTypes` namespace renamed to `MagicBytes`
- [x] Two-tier resolution in `FileService::newDiskFileModel_`: magic bytes first for binary formats, then extension check for special plaintext types, fallthrough to plaintext for everything else
- [x] Remove non-NBX file dialog filters (was causing Qt to auto-append `.txt` to Save As filenames). Save As uses no filter; user gets exactly what they type. Open dialogs can be revisited later.
- [x] Centralize how models get their extension: `FileMeta::preferredExt()` draws from path if on disk, `FileTypes::canonicalExt(kind)` if off-disk. Fnx uses the same sources (see Implementation Step 7).
- [ ] Address remaining NBX-related filters (Open Notebook, Save As Notebook, Import). These still need the `.hearthx` extension filter. Consider a small Filters header/namespace that pulls the translatable name from Tr and the extension from `Fnx::Io::EXT` (and eventually from `FileTypes` for import filters)
- [x] Generalize NBX import to accept any file type and preserve source extension. Also generalized new file creation to accept a `FileTypes::Kind`.
- [x] (Decided: keep) NBX manifest `extension` attribute. See "NBX extension attribute" section below.
- [ ] Tree view icons by file type
- [ ] Handle Notepad file renaming via TreeView
- [ ] Rename-triggered view/model re-evaluation (extension change -> new view)
- [ ] Consider NoOp for large unsupported binary files (e.g., images) that would be wasteful to open as text
- [ ] Update related docs (FileModelsAndViews, Notebooks, Architecture, Roadmap)

### Note on file dialog filters

All non-NBX file dialog filters have been removed. Save As dialogs offer no filter, so Qt does not auto-append any extension. The suggested filename (via `start_path`) already has the right extension from `preferredExtension()`, which nudges the user without forcing anything. If the user changes or removes the extension, that is their choice. Changing a file's extension changes how Hearth handles it (e.g., renaming `.md` to `.txt` makes it plain text instead of Markdown). This is intentional and consistent with other editors.

New files get their extension suggested through the pre-filled dialog name (e.g., `Untitled.txt`). If the user removes it, the file is saved with no extension and opens as plain text.

### Note on NBX extension attribute

The manifest stores each file's extension in an `extension` attribute (`<file extension=".txt" />`). This was considered for removal since the file is also stored as `{uuid}.{ext}` in the `content/` directory, which seems redundant. However, the attribute is kept because it serves as the index into the archive's content directory: `Fnx::Xml::relPath()` constructs the path from `uuid + ext` without scanning the filesystem. Removing it would require globbing for `{uuid}.*`, which is fragile and breaks for extensionless files (where the stored filename is just `{uuid}` with no extension to glob for). The attribute is populated from reality, never hardcoded: `fsPath.extQString()` on import, `FileTypes::canonicalExt(kind)` for new files.

## Resolution Strategy

File type resolution follows a **two-tier** approach: magic bytes first, then extension for special text types.

1. **Tier 1 - Magic bytes** (`MagicBytes::type()`): Binary formats (PDF, and eventually images, etc.) are detected by their file signatures regardless of extension. This means a PDF named `notes.xyz` still opens correctly as a PDF.
2. **Tier 2 - Extension check** (`FileTypes::fromPath()`): If the file has no known signature (`NoKnownSignature`), special plaintext types are matched by extension (`.md`, `.fountain`, `.fcb`, theme files, etc.) and routed to their dedicated views.
3. **Fallthrough to PlainText**: If neither tier matches (no signature, no special extension), the file opens as plain text. This is also the fallthrough for known signatures that don't have a handler yet (e.g., PNG, GIF before image viewing is implemented).

This approach ensures that any file can use any extension and still be opened correctly. Binary formats are identified by their actual content, not their name. Special text formats (which have no distinguishing bytes) rely on extension as the only available signal.

### NBX Archives

NBX files (`.hearthx`) are a special case handled at the Application level, not by FileService's type resolution. Application detects NBX files and routes them to Notebook Workspaces. Within a Notebook, individual files go through the same two-tier resolution when opened.

### Rename-Triggered Resolution

When a file is renamed and its extension changes, the system should re-evaluate which model/view pair is appropriate. For example, renaming `notes.txt` to `notes.md` should swap from a plain text view to a Markdown view. This implies FileService (or the Workspace layer) needs to detect extension changes on rename and potentially reconstruct the view, and possibly the model if the new type requires a different model class. Note that Tier 1 types (binary formats) are unaffected by rename since their resolution is based on content, not extension.

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

These types are recognized by their magic bytes (binary) or by extension (special text) and receive their own model/view pairs or specialized views.

#### PDF

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

#### Corkboard (Tentative)

A visual planning tool for organizing story elements. Corkboard files are JSON stored with a special extension. Multiple corkboard files can exist per project, and they can be saved to disk via Notepad like any other file. Detected by extension (Tier 2) since the underlying data is plaintext JSON.

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

#### Theme Editor (Long-Term)

Custom views for Hearth's own window and editor theme files. These are two distinct file types (Hearth Window theme and Hearth Editor theme) that share a similar editing approach. The underlying data is JSON. Detected by extension (Tier 2).

This is **way down the road**. The initial concept is not necessarily a fully custom view: it may be a text view augmented with properties that show color pickers beside existing value fields, where the user can still type directly and pickers pop up contextually. The view would draw on a theme API exposed from `Themes.h`.

| | |
|---|---|
| **Extensions** | Hearth window theme ext, Hearth editor theme ext (TBD) |
| **Model** | Possibly `TextFileModel` or a thin subclass |
| **View** | Augmented text view with color pickers / form fields |
| **Modification** | Yes |
| **Detection** | Tier 2 (extension) |

#### Markdown (Future)

Markdown files would receive a dedicated view (likely a rendered preview or a split edit/preview). The model may be `TextFileModel` or a subclass: the underlying data is still plain text; the distinction is in how it's displayed. Detected by extension (Tier 2).

| | |
|---|---|
| **Extension** | `.md` |
| **Model** | `TextFileModel` or subclass |
| **View** | Markdown-aware view (rendered preview, split mode, etc.) |
| **Detection** | Tier 2 (extension) |

#### Fountain (Future)

Fountain (screenwriting format) files would follow the same pattern as Markdown: plain text data with a specialized view for screenplay formatting. Detected by extension (Tier 2).

| | |
|---|---|
| **Extension** | `.fountain` |
| **Model** | `TextFileModel` or subclass |
| **View** | Fountain-aware view (screenplay formatting) |
| **Detection** | Tier 2 (extension) |

#### Diff (Stretch)

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

Previously served as a catch-all for unrecognized file types. Currently not used in the resolution flow since everything falls through to PlainText. Kept around for potential future use, e.g., for large unsupported binary files (images, etc.) where opening as text would be wasteful. May also be useful if we want a distinct "this file type will be supported eventually" placeholder.

| | |
|---|---|
| **Model** | `NoOpFileModel`: implements `data()`/`setData()` with own `QByteArray` storage. Returns whatever was loaded (may be empty). |
| **View** | `NoOpFileView`: centered face glyph (`:')`) at 0.3 opacity) |
| **Modification** | No |

## NBX Manifest and File Storage

Within a Notebook's 7-Zip archive, every file is stored in the `content/` directory named by UUID **with its real extension** (e.g., `content/{uuid}.txt`, `content/{uuid}.pdf`). Files are not stored as bare UUIDs.

The XML manifest tracks each file's metadata:

```xml
<file name="Chapter One" uuid="abc-123" extension=".txt" />
<file name="Reference" uuid="def-456" extension=".pdf" />
```

NBX accepts import of **any file type**, mirroring Notepad's open-anything philosophy.

### Extension handling

- **Import**: The extension is taken from the source file on disk via `fsPath.extQString()` (including files with no extension or custom extensions). A PDF retains `.pdf`; a file called `notes` with no extension retains no extension. Note that `std::filesystem::path::extension()` returns only the final extension, so `archive.tar.gz` stores `.gz` as the extension and `archive.tar` as the display name. This is correct: `.gz` is the format, and the full original name can be reconstructed from `name + ext` on export.
- **New file creation**: The extension comes from `FileTypes::canonicalExt(kind)` for the given file type (currently only `Plaintext`, which gives `.txt`).
- **Type resolution on open**: When a Notebook element is opened, it goes through the same two-tier resolution as any other file (magic bytes first, then extension). The stored filename's extension is available but bytes take priority for binary formats.
- **Tree view icons**: File type should determine the icon shown in the tree view, with a generic icon for unrecognized types.

### NBX files within NBX archives

It is possible to import an NBX archive into another Notebook. The file is stored like any other imported file. If opened from within the Notebook, it goes through FileService's two-tier resolution: MagicBytes detects the 7-Zip signature, but since there is no dedicated handler for 7-Zip in FileService, it falls through to plain text. The user sees binary content. This is expected.

Opening an inner NBX as a functional Notebook was considered and deliberately deferred. The implementation would require nested archive lifecycle management (extraction, save propagation, closure coordination), which conflicts with the current architecture where Workspaces are independent peers. A simpler "open as independent Notebook" approach was also considered, but it creates a confusing UX: edits to the inner Notebook would not propagate back to the outer archive, contradicting user expectations. This may be revisited if a compelling use case emerges.

## FileTypes Registry

Extensions and type metadata needed across the application (themes need theme extensions, Application needs `.hearthx`, Workspaces need `.pdf`, Tr needs type names for filter strings, etc.) should draw on a central registry.

The existing `FileTypes` namespace has been renamed to `MagicBytes` (byte-level signature detection). The `FileTypes` name is now free for a new header providing:
- A constexpr table of supported types mapped to their canonical extensions (with aliases like `.jpg` for `.jpeg`)
- `canonicalExt(Kind)`: returns the canonical extension for a type
- `fromPath(const Coco::Path&)`: resolves a file's extension to a Kind (PlainText for anything unrecognized)
- Eventually: a dynamic filter string builder (deferred due to Tr circular dependency concerns)

The relationship between `FileTypes` (registry) and `MagicBytes` (detection): they solve different problems. `MagicBytes` answers "what *is* this file?" `FileTypes` answers "what can Hearth *do* with this file?" They remain separate; FileService bridges them via the two-tier resolution.

## FileService Resolution Flow

```
newDiskFileModel_(path)
    |
    v
Tier 1: MagicBytes::type(path)
    |-- Pdf -------> PdfFileModel + PdfFileView
    |-- (future: Png, Gif, etc. -> dedicated handlers)
    |
    |-- default / NoKnownSignature:
    |       |
    |       v
    |   Tier 2: FileTypes::fromPath(path)
    |       |-- (future: Markdown, Fountain, Corkboard, themes -> dedicated views)
    |       |-- default: TextFileModel + TextFileView
    |
    v
Fallthrough: TextFileModel + TextFileView
```

## Open Questions

- **Rename view swap mechanics**: What's the right layer for detecting extension changes and triggering model/view reconstruction? Does the Workspace handle this, or FileService? Note this only affects Tier 2 types (special text), since Tier 1 types (binary) are detected by content regardless of name.
- **NoOp revival**: Should NoOp be brought back for large binary files with known signatures but no handler (e.g., a 50MB PNG)? Opening as text is technically harmless but wasteful. This is a later-problem.

## Implementation Steps

Work should happen on a **`file-types`** branch.

1. (DONE) **This document**: Establish the plan and reference for all file type work.
2. (DONE) **`MagicBytes` rename**: Existing `FileTypes` namespace renamed to `MagicBytes`. Enum renamed from `HandledType` to `Kind`, default value from `PlainText` to `NoKnownSignature`.
3. (DONE) **`AbstractFileModel` rework**: `data()` and `setData()` become pure virtual (each subclass owns its storage). `supportsModification()` becomes regular virtual defaulting to `false`. Remove `saveAs()` guard in FileService.
4. (DONE) **`FileTypes` header**: Central registry with constexpr extension table, `canonicalExt(Kind)`, and `fromPath(path)`.
5. (DONE) **Two-tier resolution in FileService**: Refactor `newDiskFileModel_` to check magic bytes first (Tier 1) for binary formats, then extension (Tier 2) for special text types, with universal fallthrough to PlainText.
6. (DONE) **Remove non-NBX file dialog filters**: Eliminates Qt auto-appending extensions on Save As. User gets exactly what they type.
7. (DONE) **Centralize model extensions**: `FileMeta::preferredExt()` draws from the file's path if on disk, or from `FileTypes::canonicalExt(kind)` if off-disk. Fnx no longer hardcodes extensions: `addNewFile` takes a `FileTypes::Kind` and resolves via `canonicalExt`, `importFile` reads the source path's extension directly.
8. **NBX filter cleanup**: Remaining NBX-related filters (Open Notebook, Save As Notebook, Import) need the `.hearthx` extension. Consider a Filters header/namespace pulling translatable names from Tr and extensions from `Fnx::Io::EXT` / `FileTypes`.
9. (DONE) **NBX all-file-type support**: `importTextFile` -> `importFile` (preserves source extension via `fsPath.extQString()`). `addNewTextFile` -> `addNewFile(FileTypes::Kind)` (resolves extension via `FileTypes::canonicalExt`). Renamed through `NbxModel` (`importFiles`, `addNewFile`) and `Notebook` call sites. No hardcoded extensions remain in Fnx. Import dialog has no filter (accepts all files).
10. **Tree view icons by type**: File-type-appropriate icons with a generic fallback for unrecognized types.

## Documents That Need Updating

Once this work lands, the following docs should be revised:

| Document | Changes needed |
|----------|----------------|
| **FileModelsAndViews.md** | Update `AbstractFileModel` API: `data()`/`setData()` now pure virtual, `supportsModification()` now regular virtual with `false` default. Add `PdfFileModel`/`PdfFileView` to concrete implementations. Update "Why Virtual Methods with Default No-Ops?" section. Note NoOp status. |
| **FileHandling.md** | Update Notebook import section to reflect all-file-type support. Note extension handling for imports and new files. |
| **Notebooks.md** | Document that NBX supports all file types, not just text. Update any `.txt`-only import references. Document `{uuid}.{ext}` file naming in archives. |
| **Architecture.md** | Mention `FileTypes` registry if it becomes a meaningful architectural element. Note `MagicBytes` rename. |
| **Roadmap.md** | Add `file-types` branch work items if not already tracked. |