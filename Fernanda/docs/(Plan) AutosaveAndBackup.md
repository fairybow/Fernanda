# Autosave and Backup

Master plan for protecting user data against bad saves and crashes. Covers pre-save backups and autosave crash recovery. Hot exit (restored from prior session) is mentioned but not planned (users should purposefully save their work and it's Fernanda's responsibility to remind them before exiting).

See: [`Workspace.h`](../src/workspaces/Workspace.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`Notebook.h`](../src/workspaces/Notebook.h), [`FileService.h`](../src/services/FileService.h), [`Fnx.h`](../src/fnx/Fnx.h), [`AppDirs.h`](../src/core/AppDirs.h), [`Time.h`](../src/core/Time.h)

Notes:

- Autosave only needs timer, most likely
- Future goal: when autosaves are detected on startup, can prompt with a diff view of autosaved version vs what is on disk if found (if current version not found, we can show a warning in that diff panel)

## Overview

There are three distinct concerns. They share some infrastructure but differ in trigger, storage, lifetime, and intent.

| Concern | Trigger | What is stored | Lifetime | Scope | Priority |
|---|---|---|---|---|---|
| Pre-save backup | Each explicit save | Copy of file/archive before overwrite | Permanent (pruned to N) | Per-file (Notepad), per-archive (Notebook) | High |
| Autosave (crash recovery) | Periodic timer | Dirty in-memory buffer content | Impermanent (deleted on clean exit) | Per-model | High |
| Hot exit / session restore | Clean shutdown / launch | Unsaved buffer state + UI layout | Persistent across sessions | Per-Workspace | Not Planned |

Pre-save backups and autosave are the two most important missing pieces. Together they cover: "I saved something I didn't mean to" (backups) and "the app died before I could save" (autosave). They are independent of each other and can be implemented in either order.

Hot exit (closing without save prompts, restoring dirty state on relaunch) and session restore (remembering open tabs and window layout) are lower priority.
Fernanda's design philosophy favors deliberate saves: users should save before closing, and close prompts are important. Hot exit may be offered as a toggleable option later. Session restore is orthogonal and deferred partly because it is a larger concern in its own right: session data could encompass anything from pinned tabs and open windows to dock positions and expanded tree items, with hot exit content being just one possible component. The non-hot-exit parts of session restore (remembering layout, pinned tabs, which Notebooks were open) are higher priority than hot exit itself and do not depend on the backup/recovery infrastructure described here.

## Definitions

- **Committed content**: data that has been explicitly saved by the user (the file on disk, or the compressed `.fnx` archive).
- **Uncommitted content**: edits that exist only in `TextFileModel`'s `QTextDocument` buffer and have not been written anywhere.
- **Working directory**: Notebook's extracted temp directory
  (`AppDirs::temp() / name~XXXXXX`). Currently, files here reflect the last extraction and new files empty files added. Eventually, we will autosave to the working dir, writing the buffer to the original files but not marking the model clean.
- **Recovery data**: serialized buffer content written to a dedicated location for the purpose of surviving a crash. Impermanent by design.
- **Autosave**: the saving of recovery data
- **Backup**: a copy of committed content made before it is overwritten. Permanent until pruned by count limit.

## Current Problem

For Notepad, a crash loses all uncommitted edits to every open file. There is nowhere to flush buffers without overwriting the user's committed files.

For Notebook, a crash also loses all uncommitted edits. The working directory holds the content as of the last explicit Notebook save (or extraction), not the current buffer state. However, the working directory also contains empty files for newly created content (created by `Fnx::Xml::addNewFile()`), whose actual content only exists in memory.

Flushing buffers to the Notebook working directory is safe because the `.fnx` archive (not the working directory) is the user's source of truth. The working directory is a transient workspace.

Notepad will use an autosave directory (possibly `~/.fernanda/notepad/autosave`).

### File creation responsibilities

`Fnx::Xml::addNewFile()` creates the physical file in the working directory because that is an FNX format concern: establishing the UUID-named file that corresponds to the new XML element. `FileService::save()` writes buffer content to that file later because FileService owns the models and their data. These are different operations at different times for different reasons. The buffer flush in autosave closes the gap naturally: after a flush, the working directory truly reflects the current in-memory state for all files (new and existing).

---

UNREVIEWED:

## Storage Layout

```
~/.fernanda/
    backups/
        notepad/
            {short-hash}_{original-filename}.{timestamp}
            {short-hash}_{original-filename}.{timestamp}
            ...
        notebooks/
            {notebook-name}.fnx.{timestamp}
            {notebook-name}.fnx.{timestamp}
            ...
    recovery/
        notepad/
            {hash}/
                buffer              (serialized content)
                meta.json           (original path, title, timestamp)
            {hash}/
                ...
            off-disk/
                {id}/
                    buffer
                    meta.json
                ...
        notebooks/
            {notebook-working-dir-name}/
                dirty.lock          (unclean shutdown marker; contains .fnx path)
```
```
AppDirs::backups()   ->  ~/.fernanda/backups/      (exists)
AppDirs::recovery()  ->  ~/.fernanda/recovery/     (new; possibly a TempDir for
                                                     automatic cleanup on
                                                     graceful exit)
```

### Key design decisions

**Flat Notepad backup directory**: Notepad backups live in a single flat
directory (`backups/notepad/`) rather than nested subdirectories. Each backup
filename encodes the source identity and the timestamp:
`{short-hash}_{original-filename}.{timestamp}`. The short hash is a truncated
hash of the absolute source path (e.g., first 8-12 hex characters of a
SHA-256), which groups backups by source file without requiring subdirectories.
The original filename is preserved for human readability. Pruning scans the flat
directory, groups files by their `{short-hash}_{original-filename}` prefix
(everything before the final `.{timestamp}`), and prunes each group to the max
count independently.

The exact naming scheme needs careful thought. Some considerations: very long
filenames could be an issue on some filesystems (255 character limit); the
original filename may contain characters that are problematic in backup
filenames; and the separator between components must be unambiguous (the
timestamp format must not conflict with characters that appear in filenames).
These details should be resolved during Phase 1 implementation.

**Notebook backups are archive-level only**: the `.fnx` archive is the atomic
unit of Notebook data. Backing up individual working directory files would be
redundant since the archive already contains the last-saved version of every
file.

**Notebook recovery via working directory**: because flushing dirty buffers to
the working directory is semantically safe (it does not alter the user's `.fnx`
source of truth), Notebook crash recovery reuses the existing working directory
rather than maintaining a separate shadow location. A lockfile marks unclean
shutdown and contains the `.fnx` path to resolve ambiguity when multiple
Notebooks share a name.

**Notepad recovery needs a shadow location**: unlike Notebook, writing a Notepad
buffer to its original file path is a destructive save. Recovery data must go to
a separate directory, keyed by a hash of the original path (matching the backup
key). Off-disk files (new, unsaved tabs) use a generated ID since they have no
path to hash.

**Recovery directory as TempDir**: the `recovery/` directory (or at least
`recovery/notepad/`) could be a `TempDir` object rather than a plain path.
`TempDir` wraps `QTemporaryDir`, which auto-removes on destruction, giving us
automatic cleanup on graceful exit for free. On crash, the destructor does not
run and the directory persists, which is exactly the desired behavior. On next
launch, Application (most likely) would check for an existing recovery directory
before creating a new one. Whether `AppDirs` returns just the path or the
`TempDir` object itself is an open question to resolve during implementation.

**Backups are permanent, recovery data is not**: backups persist until pruned by
count limit. They are the user's safety net for "I saved something I didn't mean
to." Recovery data is deleted on every clean exit. It only exists to survive
crashes.

## Phase 1: Shared Foundation

A `Backup` namespace in `core/` providing the primitives that later phases
consume.

### API sketch

```cpp
namespace Fernanda::Backup {

    // Generate a timestamp string suitable for filenames.
    // Format: YYYYMMDD-HHmmss-mmm (local time, milliseconds).
    // Uses Time::now().
    QString timestamp();

    // Short, stable hash of a path for use as a filename prefix.
    // Truncated SHA-256 hex string (e.g., 8-12 characters). Used to
    // group backups by source file in a flat directory.
    QString pathHash(const Coco::Path& path);

    // Copy `sourcePath` into `backupDir` with a generated filename:
    // {pathHash}_{original-filename}.{timestamp}
    // The hash prefix groups backups from the same source path.
    // Returns the backup path on success, empty on failure.
    // Creates `backupDir` if it does not exist.
    Coco::Path create(const Coco::Path& sourcePath, const Coco::Path& backupDir);

    // Delete the oldest entries in `backupDir` that share the same source
    // prefix as `sourcePath`, keeping at most `maxKeep`. Grouping is by
    // the {pathHash}_{original-filename} prefix (everything before the
    // final timestamp). Returns the number of entries removed.
    int prune(
        const Coco::Path& sourcePath,
        const Coco::Path& backupDir,
        int maxKeep);

    // Convenience: create a backup, then prune. Returns the backup path
    // (empty on failure).
    Coco::Path createAndPrune(
        const Coco::Path& sourcePath,
        const Coco::Path& backupDir,
        int maxKeep);

} // namespace Fernanda::Backup
```

### Deliverables

- `core/Backup.h` (header-only namespace)
- Add `AppDirs::recovery()` returning `~/.fernanda/recovery/` (may return a
  `TempDir` object instead of a plain path; to be decided during Phase 3
  implementation)
- Add `AppDirs::recovery()` creation to `AppDirs::initialize()` (or detect
  existing orphaned directory before creating a new one)
- Ensure `AppDirs::backups()` subdirectories (`notepad/`, `notebooks/`) are
  created at initialization

### Settings

- `backup/maxCount` (int, default 5): maximum backups kept per source
  file/archive
- Possibly exposed in a future Settings panel (not MVP)

### Open questions

- **Timestamp resolution**: milliseconds should be sufficient for
  human-triggered saves. Probably a non-issue in practice.
- **Backup filename scheme**: the exact format for flat-directory backup names
  (`{short-hash}_{filename}.{timestamp}`) needs to be finalized. How many hex
  characters for the hash prefix (8 is likely sufficient)? What separator
  between components (underscore risks colliding with filenames that contain
  underscores; a double-underscore or other delimiter may be clearer)? Should
  filenames be truncated to avoid filesystem length limits? Resolve during
  Phase 1 or early Phase 2.

## Phase 2: Pre-Save Backups

Before overwriting committed content, copy the original to the backup location.
Backups fire only on explicit save. Autosave does not trigger backups (Notebook
auto-flush writes to the working directory, not the archive; Notepad autosave
writes to a shadow location, not the original file).

### Notepad

**Hook point**: `FileService::writeModelToDisk_()`, before `Io::write(data,
path)`.

```cpp
// In FileService::writeModelToDisk_():
SaveResult writeModelToDisk_(AbstractFileModel* model, const Coco::Path& path)
{
    // Back up the existing file before overwriting
    if (path.exists()) {
        auto backup_dir = AppDirs::backups() / "notepad";
        Backup::createAndPrune(path, backup_dir, maxBackupCount_);
    }

    recentlyWritten_ << path.toQString();
    auto data = model->data();
    // ...
}
```

FileService needs access to the max backup count setting. It already has Bus
access and can read from SettingsService via command.

**What gets backed up**: the file as it exists on disk immediately before the
overwrite. Only files that already exist are backed up (first-time Save As has
nothing to back up).

**When it does not apply**: off-disk files being saved for the first time (no
previous version exists). `saveAs` to a path where no file previously exists.
`saveAs` to a path where a file does exist should back up that existing file
(handled naturally by the `path.exists()` check).

### Notebook

**Hook point**: `Fnx::Io::compress()` via the existing `BeforeOverwriteHook`
callback.

```cpp
// Shared hook, extracted to avoid repetition across 4 call sites:
auto makeBackupHook_() const
{
    return [](const Coco::Path& original) {
        if (!original.exists()) return;
        auto backup_dir = AppDirs::backups() / "notebooks";
        Backup::createAndPrune(original, backup_dir, maxBackupCount_);
    };
}

// Usage in save_(), saveAs_(), canCloseWindow_(), canCloseAllWindows_():
Fnx::Io::compress(path, workingDir_.path(), makeBackupHook_());
```

**What gets backed up**: the `.fnx` archive file as it exists on disk
immediately before being replaced by the new compressed archive.

**When it does not apply**: new Notebooks being saved for the first time (no
previous archive exists on disk). Save As to a new path where the original
`.fnx` is untouched.

### Failure policy

If the backup copy fails, the save proceeds anyway. A failed backup should
not prevent the user from saving their work. Log a warning.

### Deliverables

- Wire `Backup::createAndPrune` into `FileService::writeModelToDisk_`
- Finalize Notepad backup filename scheme (hash prefix length, separator,
  filename truncation for filesystem limits)
- Extract `makeBackupHook_()` helper in Notebook
- Wire `BeforeOverwriteHook` into every `Fnx::Io::compress` call site in
  Notebook (currently 4: `save_`, `saveAs_`, `canCloseWindow_`,
  `canCloseAllWindows_`)
- Read `backup/maxCount` from SettingsService
- Ensure backup does not block the save on failure (log and continue)

### Open questions

- **Disk space**: should there be a total size cap in addition to the count cap?
  For fiction writing, file sizes are small (plain text). `.fnx` archives could
  be larger if they contain images or PDFs, but 5 copies is still reasonable.
  Revisit if users report issues.

## Phase 3: Auto-Save (Crash Recovery)

Periodically flush dirty in-memory buffers to a recovery location. On clean
exit, delete the recovery data. On crash, detect orphaned recovery data at
next launch and offer to restore.

Autosave is always on. It does not change save prompts or close behavior. It
is invisible to the user unless a crash occurs. The save prompt flow on
close/quit remains exactly as it is: prompt, save/discard/cancel.

The only interaction between autosave and the close flow is cleanup: when the
user explicitly discards changes or successfully saves, any recovery data for
that file/Workspace should be removed.

### Trigger

**Primary**: periodic timer (default 30 seconds, configurable via
`autoSave/interval` setting). The timer does not fire the flush if there are no
dirty buffers (check before writing to avoid unnecessary I/O).

**Cleanup triggers**: recovery data for a file is removed when that file's
model is saved (committed), when the user discards changes (close tab/window
with "Discard"), or on clean application exit.

Autosave does not fire on tab close, window close, or focus loss. These are
normal user actions handled by save prompts. If the user discards changes at a
prompt, the recovery data is cleaned up. If the user saves at a prompt, the
committed save removes the need for recovery data.

### Write path divergence

The two Workspace types need fundamentally different write destinations for
autosave, which affects how `flushRecoveryData()` interacts with FileService.

**Notebook** flushes to the model's existing path (the working directory file).
This is the same path that `FileService::save()` writes to, so Notebook's flush
can use `FileService::save(model, ClearModified::No)`. The `ClearModified`
parameter (already planned) is the only new API surface needed.

**Notepad** cannot write to the model's existing path because that would
overwrite the user's committed file. Notepad's flush writes to a shadow
recovery location (`recovery/notepad/{hash}/buffer`). This is a completely
different path from what FileService knows about. Notepad's
`flushRecoveryData()` should bypass FileService entirely and write directly
via `Io::write(model->data(), recoveryPath)`. This is clean because:

- The recovery write is a Workspace-level concern, not a FileService concern.
- No `QFileSystemWatcher` self-write suppression is needed (the recovery path
  is not watched).
- No modification state changes should occur.
- No signals should fire.

So the two implementations diverge at the `flushRecoveryData()` level, not at
the FileService level. Notebook uses FileService (with `ClearModified::No`).
Notepad uses `Io::write` directly. No additional FileService parameters, hooks,
or path overrides are needed beyond `ClearModified`.

### Phase 3a: Notebook Buffer Persistence

Periodically flush dirty in-memory buffers to the Notebook working directory.
This is cheap, safe, and gives Notebook near-complete crash resilience with
minimal new infrastructure.

#### Mechanism

A timer calls a flush method that iterates all file models and writes dirty
buffers to the working directory. This is essentially `saveModifiedModels_()`
repurposed as a periodic operation, with one critical difference:
`saveModifiedModels_()` clears the modified flag on each model (via
`FileService::save` calling `model->setModified(false)`). The flush must not
clear the modified flag, because from the user's perspective the Notebook has
not been saved (the archive has not been recompressed).

Clearing modification triggers `modificationChanged` signals, which update tab
indicators, menu state, and the `FnxModel` edited attribute. None of that
should happen during a background flush.

#### Approach: `ClearModified` parameter on `FileService::save`

Add a `ClearModified` boolean parameter (defaulting to `Yes`) to
`FileService::save()`. The flush calls `save(model, ClearModified::No)`.

This keeps the write path centralized in FileService rather than having
Notebook bypass it with direct `Io::write` calls. It also preserves the
`QFileSystemWatcher` self-write suppression (`recentlyWritten_`), which avoids
spurious external-modification alerts. (Notebook working directory files are
in a temp directory that nothing else watches, so this is likely a non-issue,
but centralizing the write path is cleaner regardless.)

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

#### Dirty lockfile

On Notebook creation (in `setup_()`), write a `dirty.lock` file to the working
directory. The lockfile contains the absolute path to the `.fnx` archive (to
resolve ambiguity when multiple Notebooks share a name but live in different
directories).

On clean shutdown (destructor, or after successful archive compress in the
close/quit flow), delete the lockfile.

On crash, `TempDir`'s destructor does not run, so the working directory
persists with the lockfile intact. On next open of the same `.fnx`, the app
scans `AppDirs::temp()` for orphaned working directories containing a
`dirty.lock` whose stored path matches the `.fnx` being opened.

#### Recovery on launch (Notebook)

When opening a Notebook:

1. Scan `AppDirs::temp()` for directories matching the Notebook's name pattern
   that contain a `dirty.lock` file.
2. Read the `.fnx` path from each lockfile. Match against the Notebook being
   opened.
3. If a match is found, present a recovery prompt: "A previous session for
   [Notebook name] was not closed cleanly. Recover unsaved changes?"
4. If the user accepts: use the orphaned working directory instead of
   extracting from the archive. Delete the lockfile. The user sees their
   recovered state (with dirty buffers) and can save normally.
5. If the user declines: delete the orphaned working directory and extract
   fresh from the archive as usual.

This check happens in `Notebook::setup_()`, before extraction, which is the
natural point where the working directory is being prepared.

#### Timer ownership

The timer lives in Workspace base class (both types eventually need one).
Workspace defines a virtual hook that subclasses implement:

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

The timer checks whether any models are dirty before calling the hook, to avoid
unnecessary I/O when nothing has changed.

#### Deliverables

- Add `ClearModified` parameter to `FileService::save` and
  `writeModelToDisk_`
- Add `flushRecoveryData()` virtual in Workspace
- Implement Notebook override (iterate dirty models, save without clearing
  modification)
- Add `dirty.lock` creation in `Notebook::setup_()` (containing `.fnx` path)
- Add `dirty.lock` deletion in `Notebook` destructor and after successful
  compress
- Add timer to Workspace base class, wired to `flushRecoveryData()`
- Add orphaned working directory detection in `Notebook::setup_()`
- Add recovery prompt UI
- Add `autoSave/interval` setting (int, milliseconds, default 30000)
- Ensure `TempDir` does not interfere with crash persistence (verify
  `QTemporaryDir` auto-remove behavior)

#### Open questions

- **Flush during save**: if a periodic flush fires while the user is in the
  middle of a manual save, is there a race? No: everything is on the main
  thread. The timer callback will not fire until the save operation returns to
  the event loop. But worth confirming that `saveModifiedModels_()` does not
  pump the event loop internally.
- **Flush errors**: if writing a buffer to the working directory fails, the
  timer keeps trying on the next tick. Log a warning and continue.
- **Working directory cleanup**: on clean exit, `TempDir` (wrapping
  `QTemporaryDir`) cleans up the working directory. On crash, it persists. This
  is correct: the orphaned directory is the recovery data. Need to verify
  `QTemporaryDir` does not delete on crash via some OS mechanism.
- **Multiple Notebooks with same name**: the lockfile stores the full `.fnx`
  path, so two Notebooks named "MyNovel.fnx" from different directories will
  have different lockfile contents. The scan matches on path, not name.

### Phase 3b: Notepad Crash Recovery

Periodically serialize dirty Notepad buffers to a shadow recovery location. On
launch, detect orphaned recovery data and offer to restore.

#### Mechanism

Notepad's `flushRecoveryData()` implementation:

1. For each dirty `TextFileModel` with an on-disk path: write `model->data()`
   to `recovery/notepad/{pathHash}/buffer`. Write a `meta.json` alongside it
   containing the original absolute path, model title, and timestamp.
2. For each dirty off-disk model (new, unsaved tabs): write to
   `recovery/notepad/off-disk/{id}/buffer` with a `meta.json` containing the
   model's display title and timestamp. The `{id}` is a UUID generated when
   the model is created.
3. On clean exit (successful quit), delete the entire `recovery/notepad/`
   directory.

#### Cleanup on discard/save

When a file model is saved (committed) or discarded (tab closed with
"Discard"), its recovery data should be removed. This can be wired into:

- `FileService::writeModelToDisk_` (on successful write, delete recovery
  entry for that path)
- The close-tab flow (when the user discards, delete recovery entry)

This prevents stale recovery data from accumulating during a session.

#### Recovery on launch (Notepad)

At application startup, before creating the Notepad Workspace:

1. Check if `recovery/notepad/` exists and is non-empty.
2. If so, present a recovery prompt listing the files found (parsed from
   `meta.json` entries).
3. The user can choose to restore all, restore selected, or discard all.
4. For restored on-disk files: open the original path normally, then replace
   the model's buffer content with the recovery data. Mark the model as
   modified so the user can review before committing.
5. For restored off-disk files: create a new off-disk model and populate it
   with the recovery data. The tab appears as a new unsaved file.
6. Delete recovery data after processing (whether restored or discarded).

This check happens in Application, before Workspace creation, because the
recovery data exists independently of the Workspace. However, the actual model
creation and buffer replacement happen after the Notepad Workspace is created,
so the flow may need to be: detect recovery data in Application, create
Notepad, then pass recovery data to Notepad for restoration.

#### Deliverables

- Implement Notepad `flushRecoveryData()` override
- `meta.json` read/write utilities (in `Backup` namespace or a new `Recovery`
  namespace)
- Off-disk model ID generation (UUID at model creation time, stored in
  `meta.json`)
- Recovery entry cleanup on save and discard
- Recovery detection in Application startup
- Recovery prompt UI (listing recoverable files with restore/discard options)
- Clean exit cleanup (delete `recovery/notepad/`)

#### Open questions

- **Recovery prompt location**: Application detects, Notepad restores. The
  handoff needs careful sequencing: Application checks for recovery data,
  creates Notepad, then tells Notepad to restore. Alternatively, Notepad checks
  for its own recovery data during `setup_()`, similar to Notebook. The latter
  may be cleaner.
- **Partial recovery**: if only some files are recovered, the others open from
  their committed versions as usual.
- **Off-disk file identity**: recovered off-disk files are just new unsaved tabs
  with pre-populated content. They have no connection to any previous session's
  off-disk files beyond the content itself.
- **Large files**: for very large files, periodic serialization could cause UI
  hitches. Fernanda is a fiction editor, so files are typically small plain text.
  If needed later, dirty-flag checking (only write if the buffer changed since
  last flush) would help.
- **Multiple Notepad sessions**: there is only one Notepad, so the recovery
  directory is unambiguous.

## Deferred: Hot Exit

Toggleable feature where closing/quitting does not prompt to save. Instead,
unsaved buffer state is deliberately preserved and restored on next launch.
When disabled (the default and Fernanda's preferred mode), normal save prompts
apply.

This is architecturally similar to autosave crash recovery but with different
intent: autosave protects against unexpected exits, hot exit handles expected
exits by design. The storage infrastructure would be largely the same (same
write locations, same data format), but the lifecycle differs:

- Autosave data is orphaned (app crashed) and consumed once on recovery.
- Hot exit data is deliberately written on clean shutdown and expected on next
  launch.

When enabled, the close/quit flow would skip save prompts and instead write
dirty state to the recovery location, then exit. On next launch, dirty buffers
would be restored as-is. The user picks up exactly where they left off.

Implementation would build on top of Phase 3's infrastructure. Deferred because
Fernanda's design philosophy favors deliberate saves: save prompts are
important, and users should be in control of when their work is committed.

## Deferred: Session Restore

Remember what was open (tabs, windows, layout) and reopen it on next launch.
Independent of buffer state and the backup/recovery system.

Session restore is a larger concern than the features described above. It
encompasses a range of sub-features with varying priority. Layout restoration
(pinned tabs, dock positions, expanded tree items, which Notebooks were open) is
higher priority than hot exit and does not depend on the backup/recovery
infrastructure. Hot exit (persisting dirty buffer state across clean shutdowns)
is the lowest-priority sub-feature of session restore, as Fernanda's philosophy
is that users should be in the habit of saving deliberately.

### Data to persist

**Notepad**: stored in user-data `Settings.ini` or a separate session file.
Open windows with geometry and state (`QMainWindow::saveState()` /
`restoreState()`), ordered tab list per window with file paths (or off-disk
markers), active tab index per window, tree view dock state, expanded/collapsed
tree items.

**Notebook**: stored in the Notebook's working directory `Settings.ini`
(already per-Notebook). Same structure as Notepad but scoped to the Notebook.

**Application-level**: which Workspaces were active (Notepad and which
Notebooks), window stacking order.

### Features that depend on session data

- Pinned tabs
- Tab groups
- Expanded/collapsed item restoration
- Dock position restoration
- Remembering which Notebooks were open

## Implementation Order

```
Phase 1: Shared foundation (Backup namespace, AppDirs updates)
    |
    v
Phase 2: Pre-save backups (Notepad in FileService, Notebook via BeforeOverwriteHook)
    |
    v
Phase 3a: Notebook buffer persistence (timer, flush, lockfile, recovery prompt)
    |
    v
Phase 3b: Notepad crash recovery (shadow writes, cleanup, recovery prompt)
    |
    v
(Deferred) Hot exit
    |
    v
(Deferred) Session restore
```

Phases 1 and 2 are self-contained and immediately useful. Phase 3a is low-risk
and closes the biggest data-loss gap for Notebook users. Phase 3b is the most
complex piece. The deferred phases can slot in whenever priorities allow.

## Uninstaller Consideration

The Windows installer (`WindowsInstaller.iss`) currently uses `{app}\*` cleanup
before reinstall. The `~/.fernanda/` directory (user data) is separate from the
install directory, so backups are safe from reinstall. However, the uninstaller
should explicitly avoid removing `~/.fernanda/` (or at least `backups/`) to
protect user data. This needs a documentation update and possibly an
uninstaller prompt.

See Roadmap item: "Prevent uninstaller from removing .fernanda, since we'll
hold backups there."