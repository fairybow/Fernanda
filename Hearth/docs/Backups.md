# Backups

Pre-save backups that preserve the previous version of a file or archive before overwrite.

See: [`Backup.h`](../src/workspaces/Backup.h), [`FileService.h`](../src/services/FileService.h), [`Nbx.h`](../src/nbx/Nbx.h), [`Notepad.h`](../src/workspaces/Notepad.h), and [`Notebook.h`](../src/workspaces/Notebook.h).

## Overview

When the user explicitly saves, Hearth copies the existing committed content to a backup directory before overwriting it. This covers "I saved something I didn't mean to." Backups are permanent until pruned. Autosave never triggers backups.

Notepad and Notebook hook at different levels because their units of committed content differ:

| Workspace | Unit | Hook point | Backup directory |
|---|---|---|---|
| Notepad | Individual file | `FileService::beforeWriteHook` | `~/.hearth/backups/notepad/` |
| Notebook | `.hearthx` archive | `Fnx::Io::BeforeOverwriteHook` | `~/.hearth/backups/notebooks/` |

## Filename Scheme

Backups use a flat directory per Workspace type. Each filename encodes source identity and timestamp:

```
{hash}_{stem}.{timestamp}{ext}
```

Example: `a1b2c3d4_chapter-one.20260320-143022-123.txt`

- **hash**: 8-character truncated SHA-256 hex of the full source path. Groups backups by source for pruning
- **stem**: original filename without extension (human-readable identification)
- **timestamp**: `YYYYMMDD-HHmmss-mmm` in local time. Ensures uniqueness and ordering by datetime
- **ext**: original file extension, preserved so backups are immediately openable

## Hook Wiring

### Notepad

`Notepad::setup_()` sets the `FileService::beforeWriteHook`, which fires at the top of `FileService::writeModelToDisk_()` before `Io::write`. The hook skips off-disk files (no previous version to back up).

### Notebook

`Notebook::makeBackupHook_()` returns an `Fnx::Io::BeforeOverwriteHook` lambda passed to `Fnx::Io::compress()`. The hook skips new Notebooks that have not yet been saved to disk.

## Pruning

After creating a backup, `Backup::createAndPrune` lists all files in the backup directory matching the source's prefix (`{hash}_{stem}.`), sorts them by timestamp, and removes the oldest entries beyond the pruning cap.

## Failure Policy

Backup failure (copy or prune) logs a warning and continues. Saving is never blocked by a backup failure.