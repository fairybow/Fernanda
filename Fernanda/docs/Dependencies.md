# Dependencies

Fernanda relies on Qt 6 and two additional dependencies: Coco (utility library) and bit7z (7-Zip integration).

| Dependency | Type | Location | Purpose |
|------------|------|----------|---------|
| Qt 6 | External | System/vcpkg | GUI framework, XML, file I/O |
| Coco | Submodule | `Coco/` | Path and other (often) Qt-related utilities |
| bit7z | Library | `external/` | 7-Zip archive operations |

> [!IMPORTANT]
> **The following Qt extensions / additional libraries are required to build Fernanda:**
> - Qt Image Formats
> - Qt PDF

## Coco

[Coco](https://github.com/fairybow/Coco) is a personal utility library included as a Git submodule and compiled directly with Fernanda.

## bit7z

[bit7z](https://github.com/rikyoz/bit7z) provides a C++ interface to 7-Zip's compression/decompression capabilities.

Fernanda uses a [fork](https://github.com/fairybow/Fernanda-bit7z) of bit7z as a Git submodule, pinned to the v4.0.11 release. It is built from source via `add_subdirectory()` and linked as a static library (`bit7z64`).

At configure time, bit7z uses CMake's FetchContent to download the 7-Zip source code and compile it into the static library. The `BIT7Z_7ZIP_VERSION` option in [`CMakeLists.txt`](../CMakeLists.txt) controls which version is fetched (currently `25.01`). This should match the version of the runtime DLL/SO files bundled in Fernanda's resources.

### 7-Zip Runtime Library

7-Zip itself requires a dynamic library (`.dll`/`.so`) at runtime. Fernanda embeds this in the executable via Qt Resource System (`.qrc`) and copies it to user data whenever it isn't found (see [Fnx.h](../src/fnx/Fnx.h)):

- Windows: `7za.dll` (7-Zip's "alone" variant, smaller)
- Linux/Mac\*: `7z.so`

This means no external DLL to ship and no requirement for users to have 7-Zip installed.

\* `7z.so` already included but Linux/Mac releases pending!