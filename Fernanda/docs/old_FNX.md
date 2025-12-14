# FNX File Format Specification (Draft)

An `.fnx` file (Fernanda Notebook) is a 7zip archive containing a virtual file system for Notebook workspaces.

| Class/Namespace | Responsibility                                                                            |
|-----------------|-------------------------------------------------------------------------------------------|
| `Fnx`           | Defines the file type and requirements                                                    |
| `Fnx::Io`       | Archive I/O, path utilities                                                               |
| `Fnx::Xml`      | DOM element factories & queries (stateless helpers)                                       |
| `FnxModel`      | Qt model/view adapter, DOM ownership                                                      |
| `Notebook`      | Policy, working directory lifecycle, wires things together using Fnx::Io and FnxModel API |

## Archive Structure

TODO: Recoverable trash

TODO: edited to transient_edited and the clear function can just remove all attributes prefixed "transient_"

```
{name}.fnx (7zip archive)
├── Model.xml           # Virtual directory structure
└── content/            # Physical file storage
    ├── {uuid}.txt
    ├── {uuid}.txt
    └── ...
```
> [!NOTE]
> Fernanda will add a `Settings.ini` file which can be removed and regenerated as needed. It lives in the archive root but is not part of the FNX spec per se.

## Content Directory

Files in `content/` are named by UUID with a normalized extension by type, i.e.: `a1b2c3d4-e5f6-7890-abcd-ef1234567890.txt`

## `Model.xml` Schema

### Elements

| Tag        | Description                          |
|------------|--------------------------------------|
| `notebook` | Root                                 |
| `vfolder`  | Virtual folder (organizational only) |
| `file`     | Reference to a content file          |

Both `vfolder` and `file` elements may contain nested children (files can have children for outlining/hierarchy).

### Attributes

#### Common (both `vfolder` and `file`)

| Attribute | Required | Description                |
|-----------|----------|----------------------------|
| `name`    | Yes      | Display name               |
| `uuid`    | Yes      | Unique identifier          |

#### File-specific

| Attribute   | Required | Description                          |
|-------------|----------|--------------------------------------|
| `extension` | Yes      | File extension (e.g., `.txt`)        |
| `edited`    | No       | Presence indicates unsaved changes   |

> [!IMPORTANT]
> The `edited` attribute is runtime-only and must be stripped before archive compression.

#### Default Values

| Context              | Attribute | Default Value |
|----------------------|-----------|---------------|
| New file             | `name`    | `Untitled`    |
| New virtual folder   | `name`    | `New folder`  |

### Example

```xml
<?xml version="1.0"?>
<notebook>
  <vfolder name="Chapter 1" uuid="xxx1">
    <file name="1" uuid="xxx2" extension=".txt"/>
  </vfolder>
  <file name="Notes" uuid="xxx3" extension=".txt">
    <file name="Other Notes" uuid="xxx4" extension=".txt"/>
  </file>
</notebook>
```

> [!NOTE]
> This could in theory be deleted and then regenerated, but we'd lose the file names and virtual directory structure.
> TODO: However, file names could be preserved by just naming each file `name_uuid.ext` and parsing appropriately.

## `Fnx.h` Implementation

The `Fnx.h` header provides stateless utilities for working with `.fnx` archives and their extracted contents.

### Namespaces

| Namespace  | Purpose                                                  |
|------------|----------------------------------------------------------|
| `Fnx::Io`  | Archive extraction, compression, working directory setup |
| `Fnx::Xml` | DOM element creation, queries, and mutation              |

### Working Directory

All `Fnx::Io` and `Fnx::Xml` path-based operations require a **working directory** (a temporary directory where the archive is extracted for editing). Functions expect paths to be constructed relative to this root:

```
{workingDir}/
├── Model.xml
└── content/
    └── {uuid}.txt
```

Because the internal structure is rigidly defined (`content/` directory, UUID-based filenames), path utilities can reliably extract information:

```cpp
// Full physical path -> UUID
Fnx::Io::uuid(path) 
// ^ Currently returns stem (which is just the UUID). Obviously, this can be done manually, but it's easy to pass to Fnx when the path isn't specifically known at run-time

// DOM element -> Relative path
Fnx::Xml::relPath(element)  // Returns "content/{uuid}{ext}"
```

### DOM Ownership

`Fnx::Xml` functions do **not** own the `QDomDocument`. They accept it by reference for element creation and return detached elements for the caller to insert. `FnxModel` owns the DOM and uses these helpers internally.
