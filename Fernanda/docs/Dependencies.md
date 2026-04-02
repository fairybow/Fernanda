# Dependencies

Fernanda relies on Qt 6 and two additional dependencies: Coco (utility library) and miniz (ZIP file read/write).

| Dependency | Type | Location | Purpose |
|---|---|---|---|
| Qt 6 | External | System/vcpkg | GUI framework, XML, file I/O |
| Coco | Submodule | `submodules/Coco/` | Path and other (often) Qt-related utilities |
| miniz | Library | `submodules/miniz` | ZIP archive operations |
| md4c | Library | `submodules/md4c` | Markdown parsing/rendering |
| Fountain.h | Submodule | `submodules/Fountain.h` | Fountain parsing/rendering |

> [!IMPORTANT]
> **The following Qt extensions / additional libraries are required to build Fernanda:**
> - Qt Image Formats
> - Qt PDF
> - Qt WebEngine (requires Qt Position and Qt WebChannel)

(See [QtInstallerOptions.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/QtInstallerOptions.md))

## Coco

[Coco](https://github.com/fairybow/Coco) is a personal utility library included as a Git submodule and compiled directly with Fernanda.

## miniz

[miniz](https://github.com/richgel999/miniz) is a single-file C library for ZIP archive reading and writing.

Fernanda uses a [fork](https://github.com/fairybow/Fernanda-miniz) as a Git submodule, pinned to the v3.1.1 release.

## md4c

[md4c](https://github.com/mity/md4c) is a C Markdown parser (which Qt itself also uses in places).

Fernanda uses a [fork](https://github.com/fairybow/Fernanda-md4c) as a Git submodule, pinned to the v0.5.2 release.

## Fountain.h

[Fountain.h](https://github.com/fairybow/Fountain.h) is a C++ Fountain parsing/rendering "library" made for Fernanda and spun-off for general use.