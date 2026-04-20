# Recovery Autosave

Periodic flush of dirty buffers for crash recovery.

See: [`Application.h`](../src/Application.h), [`Workspace.h`](../src/workspaces/Workspace.h), [`Notebook.h`](../src/workspaces/Notebook.h), [`NotebookLockfile.h`](../src/workspaces/NotebookLockfile.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`NotepadRecovery.h`](../src/workspaces/NotepadRecovery.h), [`FileService.h`](../src/services/FileService.h), [`WorkingDir.h`](../src/workspaces/WorkingDir.h), and [`AppDirs.h`](../src/core/AppDirs.h).

## Overview

A repeating timer in `Workspace` periodically calls the virtual `autosave()` method. Subclasses override this to flush dirty buffers to a recovery location. Recovery data is deleted on clean exit, after successful saves, and when a model becomes unmodified (for example, via undo). On crash, orphaned recovery data persists and is detected on next launch.

Autosave never triggers backups, save prompts, tab indicators, or modification state changes.

| Workspace | Flush target | Recovery breadcrumb | Breadcrumb location |
|---|---|---|---|
| Notebook | Working directory (existing file paths) | `.lock` file (NBX path, working dir path, dirty UUIDs) | `~/.hearth/~temp/recovery/notebooks/` |
| Notepad | Shadow recovery directory (`AppDirs::tempNotepadRecovery()`) | Per-file recovery entries (subdirectories with buffer + meta) | `~/.hearth/~temp/recovery/notepad/` |

## Timer

`Workspace` owns a `Time::Ticker` (`autosaveCue`) connected to `virtual void autosave()`. The base implementation is empty. The timer starts at the end of `Workspace::setup_()` and fires every 15 seconds (configurable via settings, planned). Since `setup_()` runs before the subclass constructor body, the timer cannot fire until the event loop is entered, which is after subclass construction completes.

## ClearModified Parameter

`FileService::save()` and `writeModelToDisk_()` accept a `ClearModified` parameter (default `Yes`). Passing `ClearModified::No` writes the buffer to disk without calling `model->setModified(false)`, preventing autosave from triggering `modificationChanged` signals.

Notebook autosave uses `files->save(model, ClearModified::No)` rather than direct `Io::write()` because Notebook files are registered with `QFileSystemWatcher`. Direct writes would bypass the `recentlyWritten_` suppression in `writeModelToDisk_()`, causing spurious `fileModelExternallyModified` signals (and reload prompts) on every autosave tick. Notepad autosave bypasses `FileService` entirely because its recovery writes go to a separate shadow directory, not to the watched file paths.

Notebook does not set a `beforeWriteHook_`, so the backup hook in `writeModelToDisk_()` is not triggered during autosave. Notebook's backups go through `Fnx::Io::compress()` instead. If Notebook ever needs to use `beforeWriteHook_`, the solution is a separate dedicated backup hook.

## Undo-to-Clean Cleanup

Both workspaces respond to `Bus::fileModelModificationChanged` when `modified` is `false`. This prevents stale recovery data from persisting after the user undoes all changes.

Notebook: `onBusFileModelModificationChanged_()` re-saves the now-clean content to the working directory via `files->save(model, ClearModified::No)` (overwriting the stale autosaved content). On success, the UUID is removed from `recoveryDirtyUuids_`. On failure, the UUID is retained so the next lockfile tick still lists it as dirty, and a CRITICAL is logged. The next `writeLockfile_()` tick updates or removes the lockfile accordingly.

Notepad: `connectBusEvents_()` calls `deleteRecoveryEntry_(model)`, removing the recovery subdirectory and its map entry.

## Application Startup

`Application::recover_()` runs after `initializeNotepad_()` but before `handleArgs_()`. This ensures recovered Workspaces are visible before command-line args are processed, and that `handleArgs_()` can deduplicate against already-recovered Notebooks via `openOrActivateNotebook_()`.

### Notebook Recovery

Scans `AppDirs::tempNotebookRecovery()` for `.lock` files. For each lockfile, calls `Notebook::recover()`. If successful (working directory still exists), the Notebook is registered via `registerNotebook_()`, shown, and animated. If the working directory is missing, the stale lockfile is deleted.

### Notepad Recovery

Checks `AppDirs::tempNotepadRecovery()` for recovery entries. If any exist, shows the Notepad window (if not already open) and calls `Notepad::recover()`. Only calls `beCute()` if recovery caused the window to open, so that `handleArgs_()` can still own the entrance animation when there is no recovery data.

## Notebook

### Working Directory

`WorkingDir` does not auto-remove on destruction. Cleanup requires an explicit `remove()` call, ensuring the working directory survives a crash. It tracks whether it created the directory or adopted an existing one (`wasAdopted()`). Directory names use `{nbxName}~{randomSuffix}`.

`Notebook::~Notebook()` calls `clearRecoveryState_()` and removes the working directory. On crash, neither runs, and both persist.

### Autosave Flush

`Notebook::autosave()` calls `writeLockfile_()`, which:

1. If the working directory is invalid, returns
2. If the Notebook is not modified (`isModified_()` covers both file-level edits and DOM-level changes), deletes the lockfile (if present), clears `recoveryDirtyUuids_`, and returns. This makes the function self-correcting: if all files have become clean since the last tick, stale lockfiles are removed rather than silently left on disk
3. Iterates all file models, saves dirty ones via `files->save(model, ClearModified::No)`, logging failures as CRITICAL
4. Unions flushed UUIDs with `recoveryDirtyUuids_` (preserving dirty state for files not yet opened by the user)
5. Writes the lockfile via `NotebookLockfile::write()`

The lockfile path is computed by `NotebookLockfile::path()` from the recovery directory and working directory name, giving a 1:1 mapping between lockfiles and working directories.

### Lockfile Format

Plain text, one key per line:

```
nbx=/path/to/MyNovel.hearthx
working_dir=/home/user/.hearth/~temp/notebooks/MyNovel.hearthx~a1b2c3d4
dirty_uuids=3f2a1b4c-...,8e7d6c5b-...
```

Serialization and I/O are handled by the `NotebookLockfile` namespace. `Internal::toData_()` and `Internal::read_()` handle format conversion. The public API (`write()`, `read()`, `remove()`, `path()`, `readAll()`) owns all disk I/O, mirroring the structure of `NotepadRecovery`.

### Recovery State Cleanup

`clearRecoveryState_()` is the single cleanup point for all recovery artifacts:

1. Deletes the lockfile via `NotebookLockfile::remove()`
2. Clears `recoveryDirtyUuids_`
3. Nulls the `afterModelCreatedHook` (set during `applyRecoveryState_()`)

Call sites:

- After a successful `Fnx::Io::compress()` in `save_()`, `saveAs_()`, `canCloseWindow()`, and `canCloseAllWindows()`
- In the Discard branch of `canCloseWindow()` and `canCloseAllWindows()`
- In `~Notebook()` (safety net)

Additionally, `writeLockfile_()` performs partial cleanup (lockfile deletion, UUID clearing) when `isModified_()` is false, and `onBusFileModelModificationChanged_()` removes individual UUIDs when files become unmodified (conditional on write-back success).

### Recovery Construction

`Notebook::recover(lockfilePath)` is a static factory that reads the lockfile via `NotebookLockfile::read()`. It returns `nullptr` if the working directory is missing or the NBX path is empty (corrupted lockfile). Otherwise, it constructs a `WorkingDir` from the orphaned directory path (which `WorkingDir` adopts rather than creating) and calls a private constructor. `Notebook::setup_()` checks `workingDir_.wasAdopted()` to skip the normal extraction/creation step.

### Recovery Dirty State

On recovery, dirty UUIDs from the lockfile are stashed in `recoveryDirtyUuids_`. At the end of `setup_()`, `applyRecoveryState_()`:

1. Marks each UUID as edited in `NbxModel` (so the TreeView shows them dirty immediately)
2. Sets a `FileService::afterModelCreatedHook` that calls `setModified(true)` on models whose UUIDs match the set

Recovered files are born dirty: the hook fires before `connectNewModel_()`, so the first `modificationChanged` emission reports the correct state.

The hook is cleared by `clearRecoveryState_()` on first save, discard, or destruction.

## Notepad

### Autosave Flush

`Notepad::autosave()` iterates all file models, skipping unmodified ones. For each dirty model, it writes the buffer and metadata to a subdirectory of `AppDirs::tempNotepadRecovery()` via `NotepadRecovery::write()`. This bypasses `FileService` entirely (no watcher suppression needed, no signals, no backup triggers, no modification state changes).

On-disk files use a path hash as the directory name. Off-disk files need a stable directory name across autosave ticks so repeated flushes overwrite the same entry. `ensureRecoveryDir_()` is the single point for directory resolution: it returns the existing `recoveryDirs_` entry for a model, or generates and stores a new one. On-disk models get a hash-based path, off-disk models get a random name.

If a file is renamed between ticks (via the filesystem), the old path-hash entry becomes orphaned until clean exit or recovery.

### Recovery Entry Cleanup

`deleteRecoveryEntry_(model)` removes a single model's recovery subdirectory and erases it from `recoveryDirs_`. `deleteAllRecoveryEntries_()` purges all children of `tempNotepadRecovery()` and clears the map.

Cleanup call sites:

- After `singleSave_` succeeds: `save_()`, `canCloseTab()`, `canCloseTabEverywhere()`
- After `multiSave_` succeeds: iterate `result.succeeded` in `saveAllInWindow_()`, `saveAll_()`, `canCloseWindowTabs()`, `canCloseAllTabs()`, `canCloseWindow()`, `canCloseAllWindows()`
- Discard branches: single model in `canCloseTab()` / `canCloseTabEverywhere()`, `modified_models` list in the four multi-model `canClose*` methods
- `saveAs_()`
- On `modificationChanged(false)` via `connectBusEvents_()`
- `~Notepad()` (bulk, safety net)

### Recovery Read

`Notepad::recover()` reads all entries via `NotepadRecovery::readAll()` (which filters to directories only, ignoring stray files), separates them into on-disk (original file still exists) and off-disk (original missing or entry was off-disk), then caches and validates the active window before opening each file via `FileService::openFilePathIn()` or `openOffDiskTxtIn()`.

A temporary `afterModelCreatedHook` restores recovery state as each model is created: `setData()` with the recovery buffer, `setModified(true)`, and `setTitleOverride()` for off-disk entries. The hook fires before `connectNewModel_()`, so recovered models are born dirty with no signal bounce. On-disk files whose originals were deleted are treated as off-disk.

The hook captures local containers by reference, which relies on the open calls being synchronous. On-disk entries match by path key. Off-disk entries have no key, so they match by position: `takeFirst()` pairs them with `openOffDiskTxtIn()` calls in iteration order. A debug assertion after the loop verifies that all on-disk buffers were consumed by the hook.

The hook also populates `recoveryDirs_` for each recovered model (using the entry's stored `entryDir`), connecting each model to its recovery directory on disk. Recovery entries are not purged after processing: a crash before the next autosave tick would otherwise lose dirty data. The normal cleanup paths (save, discard, undo-to-clean, clean exit) handle removal.

## FileService: afterModelCreatedHook

`FileService` exposes an `afterModelCreatedHook` that fires in model creation methods after `registerModel_()` but before `connectNewModel_()`. Both Notebook and Notepad use this to establish initial model state (marking recovered files dirty) before any signals are connected.

## Namespace Symmetry

`NotebookLockfile` and `NotepadRecovery` follow the same structural pattern:

- `Entry` struct at top (public data type)
- `Internal` namespace (format keys, serialization, `read_()`)
- Public API owns all I/O (`write()`, `read()`, `remove()` / `readAll()`)
- Workspaces call the namespace for all disk operations, never using `Io` directly for recovery data

`NotepadRecovery::Entry` includes an `entryDir` field populated by `readAll()`, used during recovery to adopt existing directories into `recoveryDirs_`.

Cleanup patterns differ by necessity: Notebook uses a single `clearRecoveryState_()` helper (lockfile is one file), while Notepad uses per-model `deleteRecoveryEntry_()` and bulk `deleteAllRecoveryEntries_()` (recovery data is per-file). Both workspaces additionally clean up individual recovery artifacts when models become unmodified.