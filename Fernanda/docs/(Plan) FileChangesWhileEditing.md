# File Changes Plan

**FileMeta**

- [ ] Add `isStale` / `markStale()` / `clearStale()`
- [ ] Stale state reflected in tooltip
- [ ] Emit `changed()` when staleness toggles
- [ ] `clearStale()` available for when a stale path reappears (or future reload)

**FileService (QFileSystemWatcher)**

- [ ] Own a `QFileSystemWatcher`
- [ ] `addPath` on model registration (if on-disk)
- [ ] `removePath` on model deletion
- [ ] Swap paths (remove old, add new) on `FileMeta::pathChanged`
- [ ] On `fileChanged(path)`: check file existence, emit appropriate bus signal
- [ ] Re-add path after content-change events (watcher drops files after atomic writes on some platforms)
- [ ] `save()` refuses if `FileMeta::isStale()`

**Bus signals (new)**

- [ ] `fileModelExternallyModified(AbstractFileModel*)`: file still exists, content changed
- [ ] `fileModelPathInvalidated(AbstractFileModel*)`: file gone (deleted, moved, or unmounted)

**NotepadFileSystemModel**

- [ ] Override `dropMimeData` to capture old/new paths during drag-and-drop moves
- [ ] Emit custom `fileMoved(Coco::Path oldPath, Coco::Path newPath)` signal
- [ ] (Existing `fileRenamed` continues to handle inline renames)

**Notepad**

- [ ] Connect to `NotepadFileSystemModel::fileMoved` and call `meta->setPath(newPath)` (same pattern as existing `fileRenamed` handler)
- [ ] Connect to `bus->fileModelExternallyModified`: set tab alert ("modified externally")
- [ ] Connect to `bus->fileModelPathInvalidated`: set tab alert ("file missing"), staleness is set by FileService before the signal
- [ ] (Future: content refresh/reload popup for `fileModelExternallyModified`)

**Race condition guard**

- [ ] When `FileMeta::pathChanged` fires (from rename or move via TreeView), the watcher path swap must happen via direct connection so it completes before `QFileSystemWatcher::fileChanged` can arrive for the old path
- [ ] If needed later: a brief "ignore set" of recently-swapped paths in FileService as a safety net

**Not covered (deferred)**

- Content refresh/reload popup
- Permissions changes
- Atomic write detection (small delay to re-check before declaring stale)

---

# Old (But Maybe Useful)

| Scenario | Notepad | Notebook | Internal intercept possible | Alert | Popup |
|---|---|---|---|---|---|
| Renamed | Yes | No (UUIDs) | Yes (TreeView) | Yes | No |
| Moved | Yes | No (UUIDs) | Yes (TreeView) | Yes | No |
| Deleted | Yes | No (not yet) | Future (Trash) | Yes | No |
| Content changed externally | Yes | No (shouldn't happen) | No | Yes | Yes (refresh) |
| Permissions changed | Yes | No (working dir) | No | Maybe later | No |
| Volume unmounted | Yes | No (working dir) | No | Same as deleted | No |
| File replaced (atomic write) | Yes | No | No | Yes | Yes (refresh) |