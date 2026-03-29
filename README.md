<p align="center">
    <img src="Fernanda/resources/banner/Readme Banner.png" alt="ASCII banner" width="640">
</p>
<p align="center"><b>A plain text editor for drafting long-form fiction</b></p>
<p align="center">
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL%203-red.svg?style=for-the-badge" alt="License GPL 3"></a>
    <img src="https://img.shields.io/badge/Platform-Windows%20(x64)-blue.svg?style=for-the-badge" alt="Platform Windows (x64)">
</p>
<p align="center">
    <a href="https://github.com/fairybow/Fernanda/releases"><b>Releases</b></a> •
    <a href="Fernanda/docs"><b>Documentation</b></a>
</p>

![screenshot](Fernanda/resources/screenshots/1.apng)

Fernanda is a plain-text-first workbench for creative writing. Work on single files like a notepad or organize whole projects in [Notebooks (`.fnx`)](Fernanda/docs/Notebooks.md).

(Mac and Linux support is planned!)

## Features

Fernanda offers two workspace types: a **Notepad** for working directly on the OS filesystem and **Notebooks** for organizing whole projects inside a single, portable `.fnx` archive. Both support multiple windows, tabbed editing, and drag-and-drop.

- Distraction-free editor with line numbers, current line highlight, and selection handles
- Key filters for auto-close, smart quotes, and other typing enhancements
- Notebook file management with virtual folders, drag-and-drop tree, import, and a soft-delete trash system
- Editor themes (and user theme support via `~/.fernanda/themes` with hot reload)
- Cascading settings: per-Notebook settings inherit from Notepad defaults
- Word counter with selection-aware counting and adaptive performance
- PDF and image viewing

See [Features.md](Fernanda/docs/Features.md) for the full list.

## Installation

**First release coming soon.** (Test releases available!)

> [!IMPORTANT]
> You should not trust your writing with any version of this software less than 1.0.0 (not released yet)! Regardless, always make regular backups of your work.

## Usage

Fernanda accepts file paths as arguments. Valid `.fnx` files open in their own Notebook, and everything else opens in Notepad.

Optional flags:

- `--verbose`: enables debug-level logging to file (default is info-level and above)
- `--log-viewer`: opens an in-app log viewer window for the session

Log files are stored in `~/.fernanda/logs/` and pruned automatically.

## Built with

![C++](https://img.shields.io/badge/C++-20-blue?style=for-the-badge&logo=C%2B%2B)
[![Qt](https://img.shields.io/badge/Qt-6.10-brightgreen?style=for-the-badge&logo=qt)](https://qt.io/)
[![miniz](https://img.shields.io/badge/miniz-3.1-yellow.svg?style=for-the-badge)](https://github.com/richgel999/miniz)

(See [Dependencies.md](Fernanda/docs/Dependencies.md))

## Building

### Requirements

- C++20
- Qt 6.10.2 (with Qt Image Formats and Qt PDF)
- CMake 3.21+

### Windows (Visual Studio)

1. Install the [Qt Visual Studio Tools](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022) extension
2. Clone
3. Open the `Fernanda/` directory in Visual Studio
4. The Qt VS Tools extension will generate a `CMakeUserPresets.json` with your Qt path
5. Build and run

## Contributing

Hey! Read Fernanda's Code! (I'm proud of it.)

Fernanda prioritizes readable, maintainable code and follows conventional best practices pragmatically (not dogmatically). Throughout, simple solutions are preferred over premature abstractions. Objects and namespaces stay focused, and most files are just a few hundred lines, with about two barely exceeding 1,000. You will not find a 10,000 line `MainWindow` or anything like it.

Fernanda's [architecture](Fernanda/docs/Architecture.md) was carefully considered. I've landed on what I think is a simple but highly flexible interworking of common architectural patterns designed to keep service objects in their lanes while allowing lateral communication across a given Workspace.

But, that all said, there are always things to improve, I'm always learning, and I'll always need some help. Take a look around the repository and get to know the code. If something seems off to you or you have something new in mind, take a look at [CONTRIBUTING.md](CONTRIBUTING.md), open an issue, and we'll discuss a PR.

## License

Fernanda is free software, redistributable and/or modifiable under the terms of the [GPL 3 License](LICENSE). It's distributed in the hope that it will be useful but without any warranty (even the implied warranty of merchantability or fitness for a particular purpose).

##

I think, in part, I made this because I wanted to put off writing for as long as possible (as writers are wont to do). But I also wanted to make something all my own, something difficult yet rewarding that was way out of my realm of experience. It's taken forever, and it's still not done (it'll never be *done*-done), but it's well on its way now. Thanks for stopping by, and I hope we'll meet again.

> You have to be all kinds of stupid to say *"I can do this."*

– Linus Torvalds