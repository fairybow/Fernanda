# Dependencies

Fernanda relies on Qt 6 and two additional dependencies: Coco (utility library) and miniz (ZIP file read/write).

| Dependency | Type | Location | Purpose |
|---|---|---|---|
| Qt 6 | External | System/vcpkg | GUI framework, XML, file I/O |
| Coco | Submodule | `submodules/Coco/` | Path and other (often) Qt-related utilities |
| miniz | Library | `submodules/miniz` | ZIP archive operations |

> [!IMPORTANT]
> **The following Qt extensions / additional libraries are required to build Fernanda:**
> - Qt Image Formats
> - Qt PDF

## Coco

[Coco](https://github.com/fairybow/Coco) is a personal utility library included as a Git submodule and compiled directly with Fernanda.

## miniz

[miniz](https://github.com/richgel999/miniz) is a single-file C library for ZIP archive reading and writing.

Fernanda uses a [fork](https://github.com/fairybow/Fernanda-miniz) as a Git submodule, pinned to the v3.1.1 release. It is built from source via `add_subdirectory()` and linked as a static library.