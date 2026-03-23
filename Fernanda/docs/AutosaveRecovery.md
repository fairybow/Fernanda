# Autosave and Recovery

Periodic flush of dirty buffers for crash recovery.

See: [`Workspace.h`](../src/workspaces/Workspace.h), [`Notebook.h`](../src/workspaces/Notebook.h), [`NotebookLockfile.h`](../src/workspaces/NotebookLockfile.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`NotepadRecovery.h`](../src/workspaces/NotepadRecovery.h), [`FileService.h`](../src/services/FileService.h), [`WorkingDir.h`](../src/workspaces/WorkingDir.h), and [`AppDirs.h`](../src/core/AppDirs.h).

## Overview

A repeating timer in `Workspace` periodically calls the virtual `autosave()` method. Subclasses override this to flush dirty buffers to a recovery location. Recovery data is deleted on clean exit and after successful saves. On crash, orphaned recovery data persists and is detected on next launch.

Autosave never triggers backups. Autosave does not change save prompts, close behavior, tab indicators, or modification state.

| Workspace | Flush target | Recovery breadcrumb | Breadcrumb location |
|---|---|---|---|
| Notebook | Working directory (existing file paths) | `.lock` file (fnx path, working dir path, dirty UUIDs) | `~/.fernanda/~temp/recovery/notebooks/` |
| Notepad | Shadow recovery directory (not yet implemented) | Per-file recovery entries (not yet implemented) | `~/.fernanda/~temp/recovery/notepad/` |

## Timer

`Workspace` owns a `Time::Ticker` (`autosaveCue`) connected to `virtual void autosave()`. The base implementation is empty. The timer starts at the end of `Workspace::setup_()` and fires every 15 seconds (configurable via settings, planned). Since `setup_()` runs before the subclass constructor body, the timer cannot fire until the event loop is entered, which is after subclass construction completes.

## ClearModified Parameter

`FileService::save()` and `writeModelToDisk_()` accept a `ClearModified` parameter (default `Yes`). Passing `ClearModified::No` writes the buffer to disk without calling `model->setModified(false)`. This prevents autosave from triggering `modificationChanged` signals, which would update tab indicators, menu state, and `FnxModel` edited attributes.

## Notebook

### Working Directory

`WorkingDir` replaces the former `TempDir` (which wrapped `QTemporaryDir`). Key differences:

- No auto-removal. The destructor is a no-op. Cleanup requires an explicit `remove()` call. This ensures the working directory survives a crash for recovery
- Tracks whether it created the directory or adopted an existing one (`wasAdopted()`)
- Directory names use `{fnxName}~{randomSuffix}` (8 alphanumeric characters via `Random::token`)

`Notebook::~Notebook()` explicitly deletes the lockfile and removes the working directory. On crash, neither runs, and both persist.

### Autosave Flush

`Notebook::autosave()` calls `writeLockfile_()`, which:

1. Skips if the working directory is invalid or the Notebook is not modified (checked via `isModified_()`, which covers both file-level edits and DOM-level changes like reordering or new files)
2. Iterates all file models, saves dirty ones via `files->save(model, ClearModified::No)`
3. Collects the UUIDs of flushed models
4. Writes the lockfile to `AppDirs::tempNotebookRecovery()`

The lockfile name is derived from the working directory name (`{workingDirName}.lock`), giving a 1:1 mapping between lockfiles and working directories.

### Lockfile Format

Plain text, one key per line:

```
fnx=/path/to/MyNovel.fnx
working_dir=/home/user/.fernanda/~temp/notebooks/MyNovel.fnx~a1b2c3d4
dirty_uuids=3f2a1b4c-...,8e7d6c5b-...
```

- **fnx**: original `.fnx` archive path (resolves ambiguity for same-named Notebooks in different directories)
- **working_dir**: full path to the orphaned working directory
- **dirty_uuids**: comma-separated UUIDs of models that had unsaved changes (may be empty if only the DOM was modified)

### Lockfile Deletion

The lockfile is deleted:

- After a successful `Fnx::Io::compress()` in `save_()`, `saveAs_()`, `canCloseWindow()`, and `canCloseAllWindows()`
- In the Discard branch of `canCloseWindow()` and `canCloseAllWindows()` (the user explicitly chose to discard changes, so recovery data should not survive)
- In `~Notebook()` (redundant for the above cases but covers any other destruction path)

### Recovery Construction

`Notebook::recover(lockfilePath)` is a static factory that parses the lockfile, constructs a `WorkingDir` from the orphaned directory path (which `WorkingDir` adopts rather than creating), and calls a private constructor. `Notebook::setup_()` checks `workingDir_.wasAdopted()` to skip the normal extraction/creation step, since the working directory is already populated with flushed data.

Detection of orphaned lockfiles at startup and the recovery prompt UI are handled by Application (not yet implemented).

### Recovery Dirty State

On recovery, dirty UUIDs from the lockfile are stashed in `recoveryDirtyUuids_`. At the end of `setup_()`, `applyRecoveryState_()`:

1. Marks each UUID as edited in `FnxModel` (so the TreeView shows them dirty immediately)
2. Sets a `FileService::afterModelCreatedHook` that checks each newly created model's UUID against the set and calls `setModified(true)` if it matches

This means recovered files are born dirty: the model's modified state is set before signals are connected, so the first and only `modificationChanged` emission reports the correct state. No bounce, no redundant DOM writes.

`recoveryDirtyUuids_` is cleared alongside `deleteLockfile_()` in `save_()` and `saveAs_()`. The set seeds `writeLockfile_()` so that dirty files not yet opened by the user are preserved across subsequent autosave ticks.

## FileService: afterModelCreatedHook

`FileService` declares an `afterModelCreatedHook` (`std::function<void(AbstractFileModel*)>`) via `DECLARE_HOOK`. It fires in the private model creation methods (`newDiskFileModel_()`, `newOffDiskTextFileModel_()`) after `registerModel_()` but before `connectNewModel_()`. This lets Workspaces establish initial model state (like marking recovered files dirty) before any signals are connected or initial emissions occur.

## NotebookLockfile Namespace

`NotebookLockfile.h` centralizes lockfile serialization, replacing the inline `lockfileData_()` helper and the parsing logic formerly in `recover()`. Provides `toData()` (for writing) and `fromData()` (for reading into a `Parsed` struct containing fnx path, working dir path, and dirty UUIDs).

## NotepadRecovery Namespace

`NotepadRecovery.h` handles per-file recovery entry serialization for Notepad. Each entry is a subdirectory of `AppDirs::tempNotepadRecovery()` containing a `buffer` file and a `meta` file (original path, title).

- On-disk files use a path hash as the directory name (via `Hash::fromPath`)
- Off-disk files use `off-disk~{randomToken}` as the directory name
- `write()` creates/overwrites a single entry
- `read()` parses a single entry into an `Entry` struct
- `readAll()` scans the recovery root and returns all entries

The `Entry` struct provides `isOffDisk()` (true when `originalPath` is empty).