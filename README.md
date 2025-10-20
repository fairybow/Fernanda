# ![Colorful conch shell icon](Fernanda/resources/icons/Fernanda-32.png) Fernanda

[![License GPL 3](https://img.shields.io/badge/License-GPL%203-red.svg)](LICENSE)
![Platform Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)
[![Qt version 6.9](https://img.shields.io/badge/Qt-6.9-green.svg)](https://qt.io/)
[![Bit7z version 4.0](https://img.shields.io/badge/Bit7z-4.0-orange.svg)](https://github.com/rikyoz/bit7z)

# Description

Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)

Fernanda aims to combine cozy, distraction-free plain text editing with powerful project organization for fiction writers. Work on standalone documents in Notepad mode, or organize entire manuscripts with research materials as a self-contained Notebook kept in project-specific files (`.fnx`). Notebooks are standard 7zip archives containing your writing and materials, so they remain accessible even outside Fernanda. Planned features include multimedia viewing for research materials (PDFs, images, etc.) directly within the writing environment.

Built with Qt, Bit7z, Visual Studio, and C++.

Mac and Linux support is planned.

# Architecture

- **Workspaces**: Collections of windows and files
  - Notepad: Works with OS filesystem
  - Notebook: Planned archive-based workspaces (like binder-style applications)
- **Event-driven**: Commander/EventBus pattern for component communication
- **Modular**: Services handle core logic, modules provide optional features

```
Application
└── Workspace(s)
    ├── Bus (Commander + EventBus)
    ├── Services (Windows, Files, Views...)
    └── Modules (ColorBars, Menus, WordCounters...)
```

# Building

Open `Fernanda.vcxproj` in Visual Studio and build.

## Requirements

- Qt 6.10.0
- Visual Studio 2022
- C++ 20
- Windows only

# Status

Work in progress (no releases available).

# License

Fernanda is free and open source software. Both the application and its source code are available under the GNU GPL 3 License with additional Section 7 terms invoked. See the [LICENSE](LICENSE) and [ADDITIONAL_TERMS](ADDITIONAL_TERMS) files for complete details.