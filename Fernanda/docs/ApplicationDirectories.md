# Application Directories

Fernanda uses a few standard locations for storing data.

See: [`AppDirs.h`](../src/AppDirs.h)

## Directory Structure

| Directory | Location | Purpose |
|-----------|----------|---------|
| User Data | `~/.fernanda/` | Application data (settings, temp files, libraries) |
| Temp | `~/.fernanda/temp/` | Working directories for open Notebooks |
| Default Docs | `~/Documents/Fernanda/` | Default location for file dialogs |

These paths are managed by `AppDirs` and created at startup if they don't exist.

## User Data Contents

```
~/.fernanda/
|-- Settings.ini    # Base settings (shared defaults)
|-- temp/           # Notebook working directories
|   |-- MyNovel.fnx~abc123/
|   +-- ShortStory.fnx~def456/
+-- 7za.dll         # Extracted 7zip library (Windows)
```

## Settings Inheritance

`Settings.ini` in user data serves as the **base configuration** for all Workspaces. It contains shared defaults like editor font, word wrap preferences, etc.

Each Notebook can override these settings with its own `Settings.ini` stored inside the archive. When a Notebook opens, its settings are layered on top of the base:

```
Base settings (~/.fernanda/Settings.ini)
    +-- Notebook override (inside .fnx archive)
```

This means:
- Notepad uses the base settings directly
- Notebooks inherit base settings but can customize per-project
- A Notebook without custom settings behaves (settings-wise) identically to Notepad

The base file isn't "Notepad.ini" because it's not Notepad-specific, it's the foundation that all Workspaces build upon.

## Future Considerations

- **Backups folder**: For archive backups before saves
- **Configurable docs path**: Let users choose default save location
