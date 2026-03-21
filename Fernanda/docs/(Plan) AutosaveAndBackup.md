# Autosave and Backup

UNREVIEWED (Just for reference):

Master plan for protecting user data against bad saves and crashes.

See: [`Workspace.h`](../src/workspaces/Workspace.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`Notebook.h`](../src/workspaces/Notebook.h), [`FileService.h`](../src/services/FileService.h), [`Fnx.h`](../src/fnx/Fnx.h), [`AppDirs.h`](../src/core/AppDirs.h), [`Time.h`](../src/core/Time.h)

Notes:

- Autosave only needs timer, most likely
- Future goal: when autosaves are detected on startup, prompt with a diff view
  of autosaved version vs what is on disk (if current version not found, show a
  warning in that diff panel)

## Checklists

### Backup

- [ ] `core/Backup.h` namespace (timestamp, pathHash, create, prune,
  createAndPrune)
- [ ] Finalize backup filename scheme (hash length, separator, truncation)
- [ ] `backup/maxCount` setting (default 5)
- [ ] Notepad: wire `Backup::createAndPrune` into
  `FileService::writeModelToDisk_`
- [ ] Notebook: extract `makeBackupHook_()` helper
- [ ] Notebook: wire `BeforeOverwriteHook` into all 4 `Fnx::Io::compress` call
  sites
- [ ] Verify backup failure does not block save (log and continue)
- [ ] Create `Backup.md` official documentation

### Autosave (crash recovery)

- [ ] Add `ClearModified` parameter to `FileService::save` and
  `writeModelToDisk_`
- [ ] Add `flushRecoveryData()` virtual in Workspace
- [ ] Add timer to Workspace base class, wired to `flushRecoveryData()`
- [ ] `autoSave/interval` setting (default 30000ms)
- [ ] Notebook: implement `flushRecoveryData()` override (save without clearing
  modification)
- [ ] Notebook: `dirty.lock` creation in `setup_()` (containing `.fnx` path)
- [ ] Notebook: `dirty.lock` deletion in destructor and after successful
  compress
- [ ] Notebook: orphaned working directory detection in `setup_()`
- [ ] Notebook: recovery prompt UI
- [ ] Notepad: implement `flushRecoveryData()` override (write to shadow
  recovery dir via `Io::write`)
- [ ] Notepad: `meta.json` read/write utilities
- [ ] Notepad: off-disk model ID generation (UUID)
- [ ] Notepad: recovery entry cleanup on save and discard
- [ ] Notepad: recovery detection on launch
- [ ] Notepad: recovery prompt UI
- [ ] Notepad: clean exit cleanup (delete recovery dir)
- [ ] Verify `QTemporaryDir` does not clean up on crash
- [ ] Create `AutosaveRecovery.md` official documentation

## Overview

| Concern | Trigger | Stored | Lifetime | Scope | Priority |
|---|---|---|---|---|---|
| Backup | Explicit save | File/archive before overwrite | Permanent (pruned) | Per-file / per-archive | High |
| Autosave | Timer | Dirty buffer content | Deleted on clean exit | Per-model | High |
| Hot exit | Clean shutdown | Unsaved state + layout | Across sessions | Per-Workspace | Not Planned |

Backups cover "I saved something I didn't mean to." Autosave covers "the app
died before I could save." They are independent and can be implemented in either
order.

Hot exit and session restore are deferred. Fernanda favors deliberate saves:
close prompts are important. Session restore is a larger concern (pinned tabs,
dock positions, expanded items, which Notebooks were open) whose non-hot-exit
parts are higher priority than hot exit and do not depend on this
infrastructure.

## Definitions

- **Committed content**: explicitly saved data (file on disk, compressed `.fnx`)
- **Uncommitted content**: edits in `TextFileModel`'s `QTextDocument` buffer only
- **Working directory**: Notebook's extracted temp dir; files here reflect last
  extraction/flush, not current buffer state
- **Recovery data**: serialized buffers written for crash survival; impermanent
- **Backup**: copy of committed content before overwrite; permanent until pruned

## Current Problem

Both Workspace types share the same gap: between edits and explicit save, the
only copy of changes is in memory.

Notepad has nowhere to flush buffers without overwriting committed files.
Notebook's working directory holds content as of last save/extraction, plus
empty shells for newly created files. Flushing to the working directory is safe
because the `.fnx` archive is the source of truth.

`Fnx::Xml::addNewFile()` creates physical files (FNX format concern).
`FileService::save()` writes buffer content (data concern). Autosave's periodic
flush closes the gap: after a flush, the working directory reflects current
in-memory state.

## Storage Layout

```
~/.fernanda/
    ~temp/
        notebooks/
            {name}~XXXXXX/             (Notebook working directories)
        recovery/
            notebooks/
                dirty.lock             (contains .fnx path)
            notepad/
                {hash}/
                    buffer
                    meta.json
                off-disk/
                    {id}/
                        buffer
                        meta.json
    backups/
        notepad/
            {short-hash}_{filename}.{timestamp}
        notebooks/
            {notebook-name}.fnx.{timestamp}
    themes/

~/Documents/Fernanda/
```

### Design decisions

**Flat Notepad backup directory**: each backup filename encodes source identity
and timestamp (`{short-hash}_{original-filename}.{timestamp}`). The short hash
(truncated SHA-256) groups backups by source file. Pruning groups by prefix and
prunes each group independently. Naming details (hash length, separator,
truncation for filesystem limits) to be finalized in Phase 1.

**Notebook backups are archive-level only**: the `.fnx` archive is atomic.
Individual file backups would be redundant.

**Notebook recovery via working directory**: flushing to the working directory
is semantically safe. A `dirty.lock` in the recovery dir marks unclean shutdown
and stores the `.fnx` path (resolving ambiguity for same-named Notebooks in
different directories).

**Notepad recovery needs a shadow location**: writing a Notepad buffer to its
original path is a destructive save. Recovery data goes to a separate directory
keyed by path hash. Off-disk files use a generated ID.

**Backups are permanent, recovery data is not**: backups persist until pruned.
Recovery data is deleted on every clean exit.

## Phase 1: Shared Foundation

`core/Backup.h` namespace providing primitives for Phase 2.

```cpp
namespace Fernanda::Backup {

    // Format: YYYYMMDD-HHmmss-mmm (local time). Uses Time::now().
    QString timestamp();

    // Truncated SHA-256 hex (8-12 chars). Groups backups by source in flat dir.
    QString pathHash(const Coco::Path& path);

    // Copy sourcePath into backupDir as {pathHash}_{filename}.{timestamp}.
    // Creates backupDir if needed. Returns backup path (empty on failure).
    Coco::Path create(const Coco::Path& sourcePath, const Coco::Path& backupDir);

    // Prune oldest entries sharing sourcePath's prefix, keeping maxKeep.
    int prune(
        const Coco::Path& sourcePath,
        const Coco::Path& backupDir,
        int maxKeep);

    // Create + prune. Returns backup path (empty on failure).
    Coco::Path createAndPrune(
        const Coco::Path& sourcePath,
        const Coco::Path& backupDir,
        int maxKeep);

} // namespace Fernanda::Backup
```

Setting: `backup/maxCount` (int, default 5).

## Phase 2: Pre-Save Backups

Before overwriting committed content, copy the original. Backups fire only on
explicit save (autosave never triggers backups).

### Notepad

Hook point: `FileService::writeModelToDisk_()`, before `Io::write`.

```cpp
SaveResult writeModelToDisk_(AbstractFileModel* model, const Coco::Path& path)
{
    if (path.exists()) {
        auto backup_dir = AppDirs::backups() / "notepad";
        Backup::createAndPrune(path, backup_dir, maxBackupCount_);
    }

    recentlyWritten_ << path.toQString();
    auto data = model->data();
    // ...
}
```

Backs up the file before overwrite. Does not apply to first-time saves (no
previous version). Save As to an existing path backs up that file naturally via
the `path.exists()` check. FileService reads `backup/maxCount` from
SettingsService.

### Notebook

Hook point: `Fnx::Io::compress()` via `BeforeOverwriteHook`.

```cpp
auto makeBackupHook_() const
{
    return [](const Coco::Path& original) {
        if (!original.exists()) return;
        auto backup_dir = AppDirs::backups() / "notebooks";
        Backup::createAndPrune(original, backup_dir, maxBackupCount_);
    };
}

// All 4 call sites: save_(), saveAs_(), canCloseWindow_(), canCloseAllWindows_()
Fnx::Io::compress(path, workingDir_.path(), makeBackupHook_());
```

Does not apply to new Notebooks (no archive on disk yet).

### Failure policy

Backup failure does not block save. Log a warning and proceed.

## Phase 3: Autosave (Crash Recovery)

Periodically flush dirty buffers to a recovery location. Delete on clean exit.
Detect orphaned data on launch and offer to restore. Always on. Does not change
save prompts or close behavior.

### Trigger

Timer (default 30s, configurable). Skips flush if no dirty buffers. Recovery
data is cleaned up on committed save, discard, or clean exit.

### Write path divergence

**Notebook** flushes to the model's existing working directory path via
`FileService::save(model, ClearModified::No)`. The `ClearModified` parameter is
the only new API surface.

**Notepad** bypasses FileService and writes directly via
`Io::write(model->data(), recoveryPath)`. The recovery write is a
Workspace-level concern: no watcher suppression needed, no signals, no
modification state changes.

### Phase 3a: Notebook

Flush dirty buffers to the working directory without clearing modification
(which would trigger `modificationChanged` signals, updating tab indicators,
menu state, and `FnxModel` edited attributes).

```cpp
// FileService:
COCO_BOOL(ClearModified);

[[nodiscard]] SaveResult save(
    AbstractFileModel* fileModel,
    ClearModified clearModified = ClearModified::Yes)
{
    if (!fileModel || !fileModel->isUserEditable()) return NoOp;
    auto meta = fileModel->meta();
    if (!meta || meta->isStale()) return NoOp;
    auto path = meta->path();
    if (path.isEmpty()) return NoOp;

    return writeModelToDisk_(fileModel, path, clearModified);
}
```

Timer lives in Workspace base (both types need one). Virtual hook per subclass:

```cpp
// Workspace:
virtual void flushRecoveryData() {}

// Notebook:
void flushRecoveryData() override
{
    for (auto& model : files->fileModels()) {
        if (!model || !model->isModified()) continue;
        files->save(model, ClearModified::No);
    }
}
```

**Dirty lockfile**: created in `setup_()`, contains `.fnx` path. Deleted on
clean shutdown (destructor or after successful compress). On crash, `TempDir`'s
destructor doesn't run, so the working directory persists with the lockfile.

**Recovery**: on open, scan `AppDirs::tempNotebooks()` for orphaned working
directories with a `dirty.lock` whose stored path matches. Prompt to recover
(use orphaned directory) or discard (delete it and extract fresh). Happens in
`Notebook::setup_()` before extraction.

### Phase 3b: Notepad

Notepad's `flushRecoveryData()` writes each dirty model's buffer to
`recovery/notepad/{pathHash}/buffer` with a `meta.json` (original path, title,
timestamp). Off-disk models write to `recovery/notepad/off-disk/{uuid}/`.
On clean exit, delete `recovery/notepad/`.

**Cleanup during session**: recovery entry removed on committed save
(`writeModelToDisk_` success) and on discard (tab close with "Discard").

**Recovery on launch**: check if `recovery/notepad/` is non-empty. Prompt to
restore all, selected, or discard. Restored on-disk files open normally then
get their buffer replaced with recovery data (marked modified). Restored
off-disk files become new unsaved tabs. Recovery data deleted after processing.

Detection likely happens in `Notepad::setup_()` (similar to Notebook) rather
than Application.

### Open questions (Phase 3)

- **Flush during save**: no race (main thread), but confirm
  `saveModifiedModels_()` doesn't pump the event loop
- **Multiple same-name Notebooks**: lockfile stores full `.fnx` path, scan
  matches on path
- **Recovery prompt location**: leaning toward each Workspace's `setup_()`
- **Off-disk recovery**: recovered files are just new tabs with pre-populated
  content
- **Large files**: non-issue for fiction (plain text); dirty-flag optimization
  available later if needed

## Deferred: Hot Exit

Toggleable. When enabled, close/quit skips save prompts and writes dirty state
to recovery location. On launch, restores buffers as-is. Builds on Phase 3
infrastructure. Deferred because Fernanda favors deliberate saves.

## Deferred: Session Restore

Orthogonal to backup/recovery. Encompasses pinned tabs, dock positions,
expanded items, which Notebooks were open. Layout restoration is higher priority
than hot exit. Stored in `Settings.ini` (Notepad in user data, Notebook in
working directory, application-level for Workspace list).

## Implementation Order

```
Phase 1  ->  Phase 2  ->  Phase 3a  ->  Phase 3b  ->  (Deferred)
Foundation   Backups      Notebook      Notepad        Hot exit /
                          autosave      autosave       Sessions
```

## Uninstaller Consideration

`~/.fernanda/` is separate from install dir, so backups survive reinstall.
Uninstaller should avoid removing `~/.fernanda/` (at least `backups/`). Needs
documentation update and possibly an uninstaller prompt.