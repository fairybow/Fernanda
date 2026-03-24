# Autosave and Recovery

Periodic flush of dirty buffers for crash recovery.

See: [`Application.h`](../src/Application.h), [`Workspace.h`](../src/workspaces/Workspace.h), [`Notebook.h`](../src/workspaces/Notebook.h), [`NotebookLockfile.h`](../src/workspaces/NotebookLockfile.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`NotepadRecovery.h`](../src/workspaces/NotepadRecovery.h), [`FileService.h`](../src/services/FileService.h), [`WorkingDir.h`](../src/workspaces/WorkingDir.h), and [`AppDirs.h`](../src/core/AppDirs.h).

## Overview

A repeating timer in `Workspace` periodically calls the virtual `autosave()` method. Subclasses override this to flush dirty buffers to a recovery location. Recovery data is deleted on clean exit and after successful saves. On crash, orphaned recovery data persists and is detected on next launch.

Autosave never triggers backups, save prompts, tab indicators, or modification state changes.

| Workspace | Flush target | Recovery breadcrumb | Breadcrumb location |
|---|---|---|---|
| Notebook | Working directory (existing file paths) | `.lock` file (fnx path, working dir path, dirty UUIDs) | `~/.fernanda/~temp/recovery/notebooks/` |
| Notepad | Shadow recovery directory (`AppDirs::tempNotepadRecovery()`) | Per-file recovery entries (subdirectories with buffer + meta) | `~/.fernanda/~temp/recovery/notepad/` |

## Timer

`Workspace` owns a `Time::Ticker` (`autosaveCue`) connected to `virtual void autosave()`. The base implementation is empty. The timer starts at the end of `Workspace::setup_()` and fires every 15 seconds (configurable via settings, planned). Since `setup_()` runs before the subclass constructor body, the timer cannot fire until the event loop is entered, which is after subclass construction completes.

## ClearModified Parameter

`FileService::save()` and `writeModelToDisk_()` accept a `ClearModified` parameter (default `Yes`). Passing `ClearModified::No` writes the buffer to disk without calling `model->setModified(false)`, preventing autosave from triggering `modificationChanged` signals.

## Application Startup

`Application::recover_()` runs after `initializeNotepad_()` but before `handleArgs_()`. This ensures recovered Workspaces are visible before command-line args are processed, and that `handleArgs_()` can deduplicate against already-recovered Notebooks via `openOrActivateNotebook_()`.

### Notebook Recovery

Scans `AppDirs::tempNotebookRecovery()` for `.lock` files. For each lockfile, calls `Notebook::recover()`. If successful (working directory still exists), the Notebook is registered via `registerNotebook_()`, shown, and animated. If the working directory is missing, the stale lockfile is deleted.

### Notepad Recovery

Checks `AppDirs::tempNotepadRecovery()` for recovery entries. If any exist, shows the Notepad window (if not already open) and calls `Notepad::recover()`. Only calls `beCute()` if recovery caused the window to open, so that `handleArgs_()` can still own the entrance animation when there is no recovery data.

## Notebook

### Working Directory

`WorkingDir` does not auto-remove on destruction. Cleanup requires an explicit `remove()` call, ensuring the working directory survives a crash. It tracks whether it created the directory or adopted an existing one (`wasAdopted()`). Directory names use `{fnxName}~{randomSuffix}`.

`Notebook::~Notebook()` explicitly deletes the lockfile and removes the working directory. On crash, neither runs, and both persist.

### Autosave Flush

`Notebook::autosave()` calls `writeLockfile_()`, which:

1. Skips if the working directory is invalid or the Notebook is not modified (`isModified_()` covers both file-level edits and DOM-level changes)
2. Iterates all file models, saves dirty ones via `files->save(model, ClearModified::No)`
3. Unions flushed UUIDs with `recoveryDirtyUuids_` (preserving dirty state for files not yet opened by the user)
4. Writes the lockfile to `AppDirs::tempNotebookRecovery()`

The lockfile name is derived from the working directory name (`{workingDirName}.lock`), giving a 1:1 mapping between lockfiles and working directories.

### Lockfile Format

Plain text, one key per line:

```
fnx=/path/to/MyNovel.fnx
working_dir=/home/user/.fernanda/~temp/notebooks/MyNovel.fnx~a1b2c3d4
dirty_uuids=3f2a1b4c-...,8e7d6c5b-...
```

Serialization is handled by the `NotebookLockfile` namespace (`toData()` / `fromData()`).

### Lockfile Deletion

The lockfile is deleted:

- After a successful `Fnx::Io::compress()` in `save_()`, `saveAs_()`, `canCloseWindow()`, and `canCloseAllWindows()`
- In the Discard branch of `canCloseWindow()` and `canCloseAllWindows()`
- In `~Notebook()` (safety net)

### Recovery Construction

`Notebook::recover(lockfilePath)` is a static factory that parses the lockfile, constructs a `WorkingDir` from the orphaned directory path (which `WorkingDir` adopts rather than creating), and calls a private constructor. `Notebook::setup_()` checks `workingDir_.wasAdopted()` to skip the normal extraction/creation step.

### Recovery Dirty State

On recovery, dirty UUIDs from the lockfile are stashed in `recoveryDirtyUuids_`. At the end of `setup_()`, `applyRecoveryState_()`:

1. Marks each UUID as edited in `FnxModel` (so the TreeView shows them dirty immediately)
2. Sets a `FileService::afterModelCreatedHook` that calls `setModified(true)` on models whose UUIDs match the set

Recovered files are born dirty: the hook fires before `connectNewModel_()`, so the first `modificationChanged` emission reports the correct state.

`recoveryDirtyUuids_` is cleared alongside `deleteLockfile_()` in `save_()` and `saveAs_()`.

## Notepad

### Autosave Flush

`Notepad::autosave()` iterates all file models, skipping unmodified ones. For each dirty model, it writes the buffer and metadata to a subdirectory of `AppDirs::tempNotepadRecovery()` via `NotepadRecovery::write()`. This bypasses `FileService` entirely (no watcher suppression, no signals, no backup triggers, no modification state changes).

On-disk files use a path hash as the directory name. Off-disk files need a stable directory name across autosave ticks so repeated flushes overwrite the same entry. `Notepad` maintains a `QHash<AbstractFileModel*, Coco::Path>` (`recoveryDirs_`) that maps each model to its recovery directory, generating a new name only on first encounter.

### Recovery Entry Cleanup

`deleteRecoveryEntry_(model)` removes a single model's recovery subdirectory and erases it from `recoveryDirs_`. `deleteAllRecoveryEntries_()` purges all children of `tempNotepadRecovery()` and clears the map.

Cleanup call sites:

- After `singleSave_` succeeds: `save_()`, `canCloseTab()`, `canCloseTabEverywhere()`
- After `multiSave_` succeeds: iterate `result.succeeded` in `saveAllInWindow_()`, `saveAll_()`, `canCloseWindowTabs()`, `canCloseAllTabs()`, `canCloseWindow()`, `canCloseAllWindows()`
- Discard branches: single model in `canCloseTab()` / `canCloseTabEverywhere()`, `modified_models` list in the four multi-model `canClose*` methods
- `saveAs_()`
- `~Notepad()` (bulk, safety net)

### Recovery Read

`Notepad::recover()` reads all entries via `NotepadRecovery::readAll()`, separates them into on-disk (original file still exists) and off-disk (original missing or entry was off-disk), then opens each file via `FileService::openFilePathIn()` or `openOffDiskTxtIn()` using the active window.

A temporary `afterModelCreatedHook` restores recovery state as each model is created: `setData()` with the recovery buffer, `setModified(true)`, and `setTitleOverride()` for off-disk entries. The hook fires before `connectNewModel_()`, so recovered models are born dirty with no signal bounce. On-disk files whose originals were deleted are treated as off-disk.

The hook is nulled and recovery subdirectories are purged after all entries are processed.

## FileService: afterModelCreatedHook

`FileService` exposes an `afterModelCreatedHook` that fires in model creation methods after `registerModel_()` but before `connectNewModel_()`. Both Notebook and Notepad use this to establish initial model state (marking recovered files dirty) before any signals are connected.