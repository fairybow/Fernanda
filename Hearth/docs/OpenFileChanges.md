# Open File Changes

How Hearth detects and responds to changes in open files, including renames, moves, deletions, and external content modifications.

See: [`FileService.h`](../src/services/FileService.h), [`FileMeta.h`](../src/models/FileMeta.h), [`ViewService.h`](../src/services/ViewService.h), [`NotepadFileSystemModel.h`](../src/workspaces/NotepadFileSystemModel.h), [`Notepad.h`](../src/workspaces/Notepad.h), and [`Bus.h`](../src/workspaces/Bus.h)

## Overview

Files opened in Hearth can change while they are open. Changes can originate from inside Hearth (`TreeView` rename or drag-and-drop move) or from outside (user edits the file in another program, deletes it, moves it via the OS, or unmounts the volume). The system detects these changes and either updates the path transparently (internal changes) or alerts the user (external changes).

The detection layer lives in `FileService`, which owns a `QFileSystemWatcher` monitoring all on-disk file models. The UI response layer lives in `ViewService`, which sets tab alerts when `Bus` signals fire. Workspaces handle internal path corrections (`Notepad` connects to `NotepadFileSystemModel` signals to call `meta->setPath()`).

## Change Scenarios

| Scenario | Notepad | Notebook | Internally intercepted | Result |
|---|---|---|---|---|
| Renamed (`TreeView`) | Yes | No (UUIDs) | Yes (fileRenamed) | Path updated |
| Moved (`TreeView` drag-and-drop) | Yes | No (UUIDs) | Yes (fileMoved) | Path updated |
| Deleted externally | Yes | Unlikely | No | Stale + alert |
| Moved externally | Yes | Unlikely | No | Stale + alert |
| Content changed externally | Yes | Unlikely | No | Alert + reload prompt |
| Volume unmounted | Yes | Unlikely | No | Stale + alert (same as deleted) |
| File replaced (atomic write) | Yes | Unlikely | No | Alert (future: reload prompt) |
| Permissions changed | Yes | Unlikely | No | Deferred |

## Detection

### QFileSystemWatcher (FileService)

`FileService` owns a `QFileSystemWatcher` that monitors all on-disk file model paths. It watches and unwatches paths at three lifecycle points:

- **Registration** (`registerModel_`): if the model has an on-disk path, it is added to the watcher
- **Deletion** (`deleteModel`): the path is removed from the watcher
- **Path change** (`FileMeta::pathChanged`): the old path is removed and the new path is added (handles `TreeView` rename/move seamlessly)

When `QFileSystemWatcher::fileChanged` fires for a path, `FileService` checks whether the file still exists:

- **File exists**: content was modified externally (or replaced via atomic write). The path is re-added to the watcher (some platforms drop the watch after a modification event). `FileService` emits `bus->fileModelExternallyModified(model)`.
- **File gone**: the file was deleted, moved externally, or the volume was unmounted. `FileService` marks the `FileMeta` as stale and modified and emits `bus->fileModelPathInvalidated(model)`.

### NotepadFileSystemModel

`NotepadFileSystemModel` provides two signals for internally-initiated changes:

- `QFileSystemModel::fileRenamed` (inherited): fires when a file is renamed via the `TreeView` delegate (inline edit). Provides the directory, old name, and new name.
- `NotepadFileSystemModel::fileMoved` (custom): fires when a file is drag-and-dropped into a different directory via `TreeView`. The `dropMimeData` override captures source paths before calling the base implementation, then emits old/new path pairs for each successfully moved file.

Both signals are connected in `Notepad::setup_()` and call `meta->setPath(newPath)`, which triggers the watcher path swap in `FileService`.

## Staleness (FileMeta)

When a file disappears from disk (detected by the watcher), `FileService` marks its `FileMeta` as stale via `markStale()`. Staleness means:

- The path stored in `FileMeta` no longer points to an existing file
- `FileService::save()` refuses to write (returns `NoOp`) (`Notepad` routes stale file Save to Save As)
- `FileService::saveAs()` still works (the user picks a new path)
- The tooltip reflects the stale state

Staleness is cleared automatically by `setPath()` (if a new valid path is assigned) or manually by `clearStale()` (reserved for future reload functionality).

## UI Response (ViewService)

`ViewService` connects to both bus signals and iterates all windows and tabs to find views backed by the affected model:

- `fileModelExternallyModified`: calls `setTabAlert` with a "modified externally" message
- `fileModelPathInvalidated`: calls `setTabAlert` with a "file no longer exists" message and flags the file as modified

This mirrors how `ViewService` already handles `fileModelModificationChanged` (for the tab flag) and `fileModelMetaChanged` (for tab text and tooltip). The pattern is: `FileService` detects, `Bus` carries, `ViewService` displays.

## Threading and Signal Ordering

`QFileSystemWatcher` uses an internal thread for monitoring. When it detects a change, `fileChanged` is delivered to `FileService` (on the main thread) via a queued connection across the thread boundary. This means the signal cannot be delivered until the main thread returns to its event loop.

When a `TreeView` rename or move occurs, the entire chain is synchronous on the main thread: the model signal fires, `Notepad`'s handler calls `meta->setPath()`, which emits `pathChanged`, which triggers `FileService`'s lambda to swap the watched path. All of this completes before returning to the event loop. By the time the watcher's queued `fileChanged` signal arrives for the old path, `pathToFileModel_` no longer contains that path and the signal is harmlessly ignored.

If this assumption ever breaks (for example, if signal delivery semantics change), a brief "ignore set" of recently-swapped paths in `FileService` would serve as a safety net.

## Reload Prompt

When `fileModelExternallyModified` fires, `ViewService` sets tab alerts and then shows a `ReloadPrompt` (window-modal `QMessageBox`) parented to the first window found with a view of the affected model. The user has two options:

- **Reload**: `ViewService` emits `bus->fileModelReloadRequested(model)`. `FileService` handles this by re-reading the file from disk via `Io::read`, calling `setData()` on the model, and clearing the modified flag.
- **Keep mine**: the model is marked as modified (even if it wasn't previously), so the user knows a future save will overwrite the external changes.

In both cases, the tab alerts are cleared after the prompt is dismissed.

### Self-write suppression

`Io::write` uses `QSaveFile` (atomic write: write to temp, rename over original), which causes the watcher to fire `fileChanged` for Hearth's own saves. `FileService` suppresses this with a `recentlyWritten_` set: paths are added before writing and checked (and removed) at the top of the watcher handler. Because the write completes synchronously and the watcher signal arrives via queued connection, the path is always in the set by the time the signal is processed.

## Not Yet Implemented

- **Atomic write detection**: some programs save by deleting the file and creating a new one. The watcher may fire a "deleted" signal when the intent is "modified." A small delay (re-check after roughly 100ms) before declaring stale would handle this.
- **Permissions changes**: not currently detected. Could be a periodic check or caught at save time.