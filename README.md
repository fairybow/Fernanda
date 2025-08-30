# ![Colorful conch shell icon](Fernanda/resources/icons/Fernanda-32.png) Fernanda

[![License GPL 3](https://img.shields.io/badge/License-GPL%203-red.svg)](LICENSE)
![Platform Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)
[![Qt version 6.9](https://img.shields.io/badge/Qt-6.9-green.svg)](https://www.qt.io/)

# Description

Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)

Built with Qt, Visual Studio, and C++.

Mac and Linux support is planned.

# Architecture

- **Workspaces**: Collections of windows and files
  - Notepad: Works with OS filesystem
  - Notebook: Planned archive-based workspaces (like binder-style applications)
- **Event-driven**: Commander/EventBus pattern for component communication
- **Modular**: Services handle core logic, modules provide optional features

# Building

Open `Fernanda.vcxproj` in Visual Studio and build.

## Requirements

- Qt 6.9.1
- Visual Studio 2022
- C++ 20
- Windows only

# Status

Work in progress (no releases available).

# License

Fernanda is licensed under the GNU GPL 3 License. See the [LICENSE](LICENSE) file for more information.