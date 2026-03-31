# Qt Installer Options

```
Extensions
+-- Qt PDF
|   +-- { version }
|       +-- MSVC 2022 x64
+-- Qt WebEngine
    +-- { version }
        +-- MSVC 2022 x64

Qt
|-- { version }
|   |-- MSVC 2022 64-bit
|   +-- Additional Libraries
|       |-- Qt Image Formats
|       |-- Qt Position (required by Qt WebEngine)
|       +-- Qt WebChannel (required by Qt WebEngine)
+-- Build Tools
    +-- Qt Maintenance Tool (required)
```