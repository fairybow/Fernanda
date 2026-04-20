# Application Directories

Hearth uses a few standard locations for storing data.

See: [`AppDirs.h`](../src/core/AppDirs.h)

## Directory Structure

```
~/.hearth/
|-- ~notebooks/                 Notebook working directories
|-- ~recovery/
|   |-- notebooks/              Notebook crash recovery
|   +-- notepad/                Notepad crash recovery
|-- backups/
|   |-- notebooks/              Per-archive backups before overwrite
|   +-- notepad/                Per-file backups before overwrite
|-- logs/
+-- themes/

~/Documents/Hearth/           Default location for file dialogs
```

These paths are managed by `AppDirs` and created on demand. Recovery and working directories are cleaned up on exit via `AppDirs::cleanup()`.

## Settings Inheritance

`Settings.ini` in user data serves as the **base configuration** for all Workspaces. It contains shared defaults like editor font, word wrap preferences, etc.

Each Notebook can override these settings with its own `Settings.ini` stored inside the working directory (and copied to the archive on save). When a Notebook opens, its settings are layered on top of the base:

```
Base settings (~/.hearth/Settings.ini)
    +-- Notebook override (in working directory)
```

This means:
- Notepad uses the base settings directly
- Notebooks inherit base settings but can customize per-project
- A Notebook without custom settings behaves (settings-wise) identically to Notepad, apart from a few Workspace-specific settings (like TreeView visibility)

The base file is not called "Notepad.ini" because it is not Notepad-specific. Rather, it's the foundation that all Workspaces build upon.

## Backups

The `backups/` subdirectories store pre-save copies of committed content. (See [Backups.md].)

## Future Considerations

- **Configurable docs path**: let users choose default save location