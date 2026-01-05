# Dependencies

Fernanda relies on Qt 6 and two additional dependencies: Coco (utility library) and bit7z (7zip integration).

| Dependency | Type | Location | Purpose |
|------------|------|----------|---------|
| Qt 6 | External | System/vcpkg | GUI framework, XML, file I/O |
| Coco | Submodule | `Coco/` | Path utilities, logging, helpers |
| bit7z | Library | `external/` | 7zip archive operations |

## Coco

[Coco](https://github.com/fairybow/Coco) is a personal utility library included as a Git submodule and compiled directly with Fernanda.

## bit7z

[bit7z](https://github.com/rikyoz/bit7z) provides a C++ interface to 7zip's compression/decompression capabilities.

Fernanda uses bit7z as a **static library**: the public headers (in [`external/`](../external)) are compiled with Fernanda and link against a pre-built `.lib` file.

### 7zip Runtime Library

7zip itself requires a dynamic library (`.dll`/`.so`) at runtime. Fernanda embeds this in the executable via Qt Resource System (`.qrc`) and copies it to user data whenever it isn't found (see [Fnx.h](../src/Fnx.h)):

- Windows: `7za.dll` (7zip "alone" variant, smaller)
- Linux/Mac*: `7z.so`

This means no external DLL to ship and no requirement for users to have 7zip installed.

* `7z.so` already included but implementation pending!
