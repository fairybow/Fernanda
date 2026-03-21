# Application Directories

Fernanda uses a few standard locations for storing data.

See: [`AppDirs.h`](../src/core/AppDirs.h)

## Directory Structure

```
~/.fernanda/
|-- ~temp/
|   |-- notebooks/              Notebook working directories
|   +-- recovery/
|       |-- notebooks/          Notebook crash recovery
|       +-- notepad/            Notepad crash recovery
|-- backups/
|   |-- notepad/                Per-file backups before overwrite
|   +-- notebooks/              Per-archive backups before overwrite
+-- themes/

~/Documents/Fernanda/           Default location for user documents
```

These paths are managed by `AppDirs` and created at startup via `AppDirs::initialize()`. Temp and recovery directories are cleaned up on exit via `AppDirs::cleanup()`.

## Settings Inheritance

`Settings.ini` in user data serves as the **base configuration** for all Workspaces. It contains shared defaults like editor font, word wrap preferences, etc.

Each Notebook can override these settings with its own `Settings.ini` stored inside the working directory (and copied to the archive on save). When a Notebook opens, its settings are layered on top of the base:

```
Base settings (~/.fernanda/Settings.ini)
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

- **Crash recovery**: timer-based flush of dirty buffers to the recovery directories (planned)
- **Configurable docs path**: let users choose default save location