/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <algorithm>
#include <functional>

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QtTypes>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/FileTypes.h"
#include "core/Io.h"
#include "core/PathKey.h"
#include "core/VaultTreeModel.h"
#include "models/AbstractFileModel.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"

namespace Fernanda {

// Every open file's buffer, one per file however many vaults and windows show
// it: a map of buffers keyed by PathKey, a watcher over their files, and the
// autosave that writes them. GUI-free. App owns the one store and lends it to
// every Vault and VaultWindow.
//
// Which files a window may open is the window's to decide
// (VaultWindow::vaultOf_); the store holds whatever it's asked to. A Vault's
// rename, move, and trash tell the store what moved or went, so every buffer
// at or under an entry follows it, whichever vault's window shows it. Around
// those disk operations the store also lets go of every vault tree's folder
// watches inside the entry, since vaults can nest and another vault's tree
// may hold one (unwatchAtOrUnder says why that matters).
//
// The store owns all buffer IO. Models are pure buffers: the store reads bytes
// and hands them to setData() on open, and reads data() back to write. One
// place writes, so one place suppresses the watcher's self-write.
//
// See docs/Architecture.md, "Saving" and "Watching the filesystem".
class BufferStore : public QObject
{
    Q_OBJECT

public:
    explicit BufferStore(QObject* parentApp)
        : QObject(parentApp)
    {
        setup_();
    }

    ~BufferStore() override { TRACER; }

    // Asked by openModel when a text file doesn't decode cleanly as UTF-8: true
    // opens it anyway, false refuses the open. Given the file's key and the raw
    // bytes, so a GUI caller can name the file and preview the damage. A plain
    // callable keeps the store GUI-free — the window supplies the prompt
    using ConfirmLossyOpen =
        std::function<bool(const PathKey& key, const QByteArray& data)>;

    // Return the existing buffer for this file, or create one. This is the
    // dedup point that makes "same file open twice" and "common file open in
    // two windows" both resolve to a single model.
    //
    // Returns nullptr for a file Fernanda won't open: an empty key, an
    // unsupported type, a file it can't read — gone since it was listed, or
    // locked — or a text file that isn't valid UTF-8 whose open the caller
    // declined. This is the AUTHORITATIVE refusal: the tree and the
    // FileSwitcher hide unsupported files, but those are advisory, and this is
    // the one gate every open path — tree, switcher, drop, restore, and later
    // CLI — funnels through. Callers treat null as "nothing to open".
    //
    // confirmLossyOpen is asked only on a fresh read that fails to decode — a
    // dedup hit returns above it, so an open buffer never prompts twice (a
    // common file opened from a second window included)
    [[nodiscard]] AbstractFileModel*
    openModel(const PathKey& key, const ConfirmLossyOpen& confirmLossyOpen)
    {
        if (key.isEmpty()) {
            WARN("Refusing to open a file with no path!");
            return nullptr;
        }

        if (auto it = models_.constFind(key); it != models_.constEnd()) {
            return it.value();
        }

        // Refuse BEFORE the read: an unsupported file may be large (a video)
        // and there is no reason to pull it into memory just to discard it
        auto type = FileTypes::typeOf(key.path());
        if (type == FileTypes::Type::Unsupported) {
            WARN("Refusing to open unsupported file type: {}", key.path());
            return nullptr;
        }

        // Refuse what can't be read — deleted since it was listed (a switcher
        // snapshot, a tree click racing a delete) or locked by another
        // program. A buffer built over nothing opens empty, and the first
        // keystroke would autosave it back into existence, or over the locked
        // original once the lock clears. tryRead, unlike read, tells an empty
        // file from a failed read, and has already logged which failure
        auto data = Io::tryRead(key.path());
        if (!data) {
            WARN("Refusing to open unreadable file: {}", key.path());
            return nullptr;
        }

        // Text that isn't valid UTF-8 — usually an older encoding like
        // Windows-1252, or a binary under a text extension. Fernanda does no
        // conversion, so the caller decides: open anyway, or refuse. Asked
        // before a model exists, so a declined file never becomes a buffer
        auto decodes_lossily =
            type == FileTypes::Type::Text && !TextFileModel::isValidUtf8(*data);

        if (decodes_lossily && !confirmLossyOpen(key, *data)) {
            INFO("Declined opening non-UTF-8 file: {}", key.path());
            return nullptr;
        }

        // Dispatch by file type. The dedup above means one buffer per file
        // regardless of type
        auto* model = makeModel_(key, type);

        model->setData(*data);
        models_.insert(key, model);

        // Start watching now that a buffer exists. Removed again on eviction or
        // external deletion; dropped and re-added around our own writes in
        // writeModel_ so a save isn't read back as an external change
        watcher_->addPath(key.path().toQString());

        // An accepted lossy open is committed NOW, not on the first edit: the
        // write replaces the undecodable bytes with the U+FFFD the buffer
        // already shows, so disk becomes valid UTF-8 and matches the buffer —
        // and every later open, eviction, and workspace restore sees a clean
        // file with nothing to ask. Marked dirty first because setData left it
        // clean and writeModel_ leaves the flag alone on failure: a failed
        // write must stay dirty so the exit sweeps retry it, not sit clean and
        // unsaved where no autosave pass looks
        if (decodes_lossily) {
            model->setModified(true);
            writeModel_(model);
        }

        // Every edit re-arms the debounce. Connected AFTER setData so the
        // initial load — which emits contentChanged through setText — doesn't
        // arm it. (Harmless if it did: writes are isModified-guarded and
        // setData left the buffer clean.) All models feed one pair of timers,
        // so any edit anywhere drives the shared debounce/ceiling
        connect(
            model,
            &AbstractFileModel::contentChanged,
            this,
            &BufferStore::onModelContentChanged_);

        // Last view closed → flush and free the buffer. The base model counts
        // its own views (GUI-free) and fires this on the 1->0 edge; the lambda
        // re-derives the map key inside evictModel_
        connect(model, &AbstractFileModel::lastViewClosed, this, [this, model] {
            evictModel_(model);
        });

        DEBUG(
            this,
            "Opened model for {} (total: {})",
            key.path(),
            models_.size());

        return model;
    }

    // Write every modified buffer back to disk. Returns the keys of the files
    // that FAILED to write (empty == all good; a buffer whose file is gone from
    // disk doesn't count), so a caller can name them.
    //
    // Synchronous, and skips buffers already matching disk. Used by App's quit
    // and focus-loss flushes; the while-typing autosave uses autosave_ below.
    // Deliberately does NOT touch the autosave timers — this is pure buffer IO
    QList<PathKey> flush()
    {
        QList<PathKey> failed{};

        for (auto* model : models_) {
            if (flushOne_(model) == WriteResult_::Failed) {
                failed << model->key();
            }
        }

        return failed;
    }

    // The same, for only these files: the ones a closing window shows
    // (VaultWindow::closeEvent, which names any failure in the one surviving
    // prompt and refuses the close), so a file only another window shows can't
    // hold this one open. A key with no buffer is passed over
    QList<PathKey> flush(const QList<PathKey>& keys)
    {
        QList<PathKey> failed{};

        for (const auto& key : keys) {
            auto* model = models_.value(key);

            if (model && flushOne_(model) == WriteResult_::Failed &&
                !failed.contains(key)) {
                failed << key;
            }
        }

        return failed;
    }

    // --- Following a Vault's file operations ---------------------------------
    //
    // A Vault renames, moves, and trashes entries on disk; these keep the
    // buffers at or under an entry in step, and every vault's tree with them,
    // since vaults can nest and any of them may show the entry. Each works on
    // every buffer at or under the key, so a folder's whole contents follow it

    // A vault's file tree, which the functions below reach along with the
    // buffers. Every Vault adds its own at construction; it leaves the list
    // when it's destroyed
    void addTree(VaultTreeModel* tree)
    {
        trees_ << tree;

        connect(tree, &QObject::destroyed, this, [this, tree] {
            trees_.removeOne(tree);
        });
    }

    // Drop every watch inside the entry before the disk op. The buffers'
    // watches, so the entry's disappearance isn't read as an outside deletion
    // that closes its views. And every tree's folder watches, the acting
    // vault's and any other's: on Windows each is a directory handle, and a
    // handle inside a folder stops it being renamed, moved, or trashed,
    // whichever vault's window holds it.
    //
    // rewatchAtOrUnder undoes it all when the disk op fails. When it succeeds,
    // applyRelocation or applyRemoval updates every tree, which watches its
    // folders again where they now are, and re-arms the buffers' watches
    void unwatchAtOrUnder(const PathKey& entry)
    {
        for (auto* model : modelsAtOrUnder_(entry)) {
            watcher_->removePath(model->key().path().toQString());
        }

        for (auto* tree : trees_) {
            tree->releaseWatches(watchedPathIn_(tree, entry));
        }
    }

    void rewatchAtOrUnder(const PathKey& entry)
    {
        for (auto* model : modelsAtOrUnder_(entry)) {
            watcher_->addPath(model->key().path().toQString());
        }

        for (auto* tree : trees_) {
            tree->restoreWatches(watchedPathIn_(tree, entry));
        }
    }

    // After the disk op moved oldEntry to newEntry. Every tree moves its row
    // (VaultTreeModel::applyRelocation), so each window keeps the entry and
    // what's under it expanded and selected; left to find out from its own
    // watcher, a tree would re-list the folder and see a new entry, collapsed.
    // A tree with no row there, old or new, does nothing. Then every buffer at
    // or under the entry takes its new key, and its views retitle. Trees
    // first, so views already show the new place when the retitles fan out.
    //
    // A tree whose vault folder was the entry, or inside it, has no row for
    // it: the whole vault went with the entry. After a change of letter case
    // alone, where case is ignored, its folder is still there and its watches
    // come back here. Either way App moves its Vault to the new place when it
    // hears entryRelocated, announced last, and the tree watches again from
    // there (Vault::moveRoot)
    void applyRelocation(const PathKey& oldEntry, const PathKey& newEntry)
    {
        for (auto* tree : trees_) {
            auto root = tree->root();

            if (PathKey::of(root).isAtOrUnder(oldEntry)) {
                if (root.isDir()) {
                    tree->restoreWatches(root);
                }

                continue;
            }

            tree->applyRelocation(
                spelledIn_(tree, oldEntry),
                spelledIn_(tree, newEntry));
        }

        for (auto* model : modelsAtOrUnder_(oldEntry)) {
            rekeyModel_(model, oldEntry, newEntry);
        }

        emit entryRelocated(oldEntry, newEntry);
    }

    // Write every modified buffer at or under the entry, for whatever reads
    // the files from disk next: a trash, so the trashed copy holds the latest
    // text, and Git, so a status sees the text on screen. A viewless dirty
    // buffer (a failed eviction flush) gets its retry here. False when any
    // write fails, having logged which; the buffers after it are left
    // unwritten. A buffer whose file is already gone isn't a failure. Before
    // a trash, call this before unwatchAtOrUnder: a write drops and re-adds
    // its own watch, which would undo an earlier unwatch
    [[nodiscard]] bool flushAtOrUnder(const PathKey& entry)
    {
        for (auto* model : modelsAtOrUnder_(entry)) {
            if (flushOne_(model) == WriteResult_::Failed) {
                WARN(
                    "Couldn't save {} while saving everything in {}!",
                    model->key().path(),
                    entry.path());
                return false;
            }
        }

        return true;
    }

    // While something outside Fernanda rewrites files at or under the entry
    // (a Git switch, pull, or discard), autosave leaves their buffers alone:
    // typing goes on in the buffer, unwritten, so it can't land over what the
    // other program writes. Without this, an autosave between that program's
    // write and the watcher's reconcile would write the old text back over
    // it, and the watcher would take that write for its own. Writes asked for
    // outright (a flush, a window closing, a tab's last view closing) still
    // happen: the text must never be dropped. Holds may overlap, from two
    // windows over one repository; each hold is released once.
    //
    // What each held buffer's file holds now is noted, so the release can
    // tell which files changed during the hold
    void holdWritesAtOrUnder(const PathKey& entry)
    {
        Hold_ hold{};
        hold.entry = entry;

        for (auto* model : modelsAtOrUnder_(entry)) {
            if (auto disk = Io::tryRead(model->key().path())) {
                hold.diskHashes.insert(model->key(), qHash(*disk));
            }
        }

        holds_ << hold;
    }

    // The end of a hold. Each held file that changed during it is first
    // taken in as the watcher would take it (reconcileOne_): adopted as one
    // undoable change, so typing made meanwhile is what Ctrl+Z brings back.
    // A file that didn't change is left alone, so typing in it stays. Then
    // autosave writes what's left modified. A buffer opened during the hold
    // has nothing noted, and is left to the watcher
    void releaseWritesAtOrUnder(const PathKey& entry)
    {
        auto it = std::find_if(
            holds_.cbegin(),
            holds_.cend(),
            [&entry](const Hold_& hold) { return hold.entry == entry; });

        if (it == holds_.cend()) {
            return;
        }

        auto hold = *it;
        holds_.erase(it);

        Coco::PathList changed{};

        for (auto h = hold.diskHashes.cbegin(); h != hold.diskHashes.cend();
             ++h) {
            auto disk = Io::tryRead(h.key().path());

            if (!disk || qHash(*disk) != h.value()) {
                changed << h.key().path();
            }
        }

        for (const auto& changed_path : changed) {
            reconcileOne_(changed_path);
        }

        for (auto* model : modelsAtOrUnder_(entry)) {
            if (model->isModified()) {
                onModelContentChanged_();
                break;
            }
        }
    }

    // After a trash: the entry's row leaves every tree, and the buffers at or
    // under it follow their files, so every view on them closes, in every
    // window. A tree whose vault folder was inside the entry has no row for
    // it and does nothing; App closes that vault's window when it hears
    // entryRemoved, announced last. Returns how many buffers went
    qsizetype applyRemoval(const PathKey& entry)
    {
        for (auto* tree : trees_) {
            tree->applyRemoval(spelledIn_(tree, entry));
        }

        auto affected = modelsAtOrUnder_(entry);

        for (auto* model : affected) {
            discardModel_(model);
        }

        emit entryRemoved(entry);
        return affected.size();
    }

    // Adjust the while-typing autosave: the debounce (write this long after
    // typing pauses) and the ceiling (write ANYWAY this long into unbroken
    // input). Not a saved setting yet
    void setAutosaveTiming(int debounceMs, int ceilingMs)
    {
        autosaveDebounceMs_ = debounceMs;
        autosaveCeilingMs_ = ceilingMs;
    }

signals:
    // A rename or move took oldEntry to newEntry, and every tree and buffer
    // has followed it: a Vault's, or App's rename of a vault's folder from
    // Manage Vaults. The entry may have been, or held, a vault's folder: App
    // moves that vault to follow it
    void entryRelocated(const PathKey& oldEntry, const PathKey& newEntry);

    // The same for a trash, once every tree and buffer has let the entry go.
    // A vault whose folder was in it has its window closed by App
    void entryRemoved(const PathKey& entry);

    // A buffer was written to its file: an autosave, a flush, any save.
    // Emitted from inside the write, which may be inside a caller's flush,
    // so a slot that would save or flush again should be queued
    void fileWritten(const PathKey& key);

private:
    // Keyed by each file's PathKey. Models are parented to the store. A model
    // is freed when its file is deleted on disk (external deletion, below) or
    // trashed in-app (applyRemoval) — both through discardModel_ — or when
    // its last view closes, in any window (eviction — evictModel_); otherwise
    // it lives as long as the store
    QHash<PathKey, AbstractFileModel*> models_{};

    // Watches the absolute path of every open file. Only files we hold buffers
    // for — the trees' folders are each VaultTreeModel's own watcher's concern
    QFileSystemWatcher* watcher_ = new QFileSystemWatcher(this);

    // Every Vault's tree (addTree), for its folder watches around a disk op
    QList<VaultTreeModel*> trees_{};

    // Self-write suppression. writeModel_ records a hash of the bytes it wrote
    // here, keyed like models_. The reconcile pass compares disk against
    // THIS fingerprint, not the live buffer — so our own save is recognized
    // however far typing has since moved the buffer on, and a genuine later
    // change (different bytes, different hash) is never mistaken for it. A
    // one-shot "ignore the next event" flag would not do: on a platform that
    // sends no echo it lingers and eats the next real change, where a
    // fingerprint can't be "used up". Entries are dropped when a buffer is
    // discarded or evicted
    QHash<PathKey, size_t> lastWrittenHash_{};

    // An entry whose buffers autosave leaves alone (holdWritesAtOrUnder),
    // and what each held buffer's file held when the hold began
    struct Hold_
    {
        PathKey entry{};
        QHash<PathKey, size_t> diskHashes{};
    };

    // One per hold, so overlapping holds of one entry each count
    QList<Hold_> holds_{};

    [[nodiscard]] bool isHeld_(const AbstractFileModel* model) const
    {
        const auto key = model->key();

        return std::any_of(
            holds_.cbegin(),
            holds_.cend(),
            [&key](const Hold_& hold) { return key.isAtOrUnder(hold.entry); });
    }

    // Watcher events are coalesced here and processed one debounce later, so a
    // git checkout or Dropbox sync rewriting many watched files becomes one
    // reconcile pass, not N — and one collapse sweep, not many. Absolute paths,
    // as QFileSystemWatcher reports them
    QSet<QString> pendingChanged_{};
    Coco::Time::Debouncer* watchReconcileTimer_ =
        Coco::Time::newDebouncer(this, &BufferStore::reconcileChanges_);

    // Long enough to swallow a burst of filesystem events, short enough to feel
    // immediate
    int watchReconcileMs_ = 250;

    // Hybrid autosave. The debounce restarts on every edit and fires on the
    // pause; the ceiling is armed once per burst and never restarted, so it
    // still fires under input that never pauses — a held key. Both single-shot,
    // both call autosave_, which stops both — so whichever fires first wins the
    // burst
    Coco::Time::Debouncer* autosaveDebounceTimer_ =
        Coco::Time::newDebouncer(this, &BufferStore::autosave_);
    Coco::Time::Debouncer* autosaveCeilingTimer_ =
        Coco::Time::newDebouncer(this, &BufferStore::autosave_);

    // The numbers aren't settled. 1000 ms coalesces a typing burst; the 3000 ms
    // ceiling bounds worst-case unsaved time under unbroken input. A writer's
    // inter-keystroke gaps are usually under the debounce, so the ceiling — not
    // the debounce — sets the save cadence during active writing; tune the
    // ceiling to change that
    int autosaveDebounceMs_ = 1000;
    int autosaveCeilingMs_ = 3000;

    void setup_()
    {
        // fileChanged drives external reload and deletion — coalesced, since
        // one git checkout can fire many at once
        connect(
            watcher_,
            &QFileSystemWatcher::fileChanged,
            this,
            &BufferStore::onWatchedFileChanged_);
    }

    // The concrete-model factory. The type is decided by FileTypes::typeOf —
    // extension-based, like Obsidian's own dispatch; no IO decides it, and
    // magic-byte sniffing buys little for the types we open. There is no
    // fallthrough to text: openModel has already refused Unsupported, so
    // reaching that case here is a bug
    [[nodiscard]] AbstractFileModel*
    makeModel_(const PathKey& key, FileTypes::Type type)
    {
        switch (type) {
        case FileTypes::Type::Text:
            return new TextFileModel(key, this);
        case FileTypes::Type::Pdf:
            return new PdfFileModel(key, this);
        case FileTypes::Type::Image:
            return new ImageFileModel(key, this);
        case FileTypes::Type::Unsupported:
            break;
        }

        ASSERT(false, "makeModel_ reached with an unsupported type!");
        return nullptr;
    }

    // Open buffers whose file is AT entry (a file relocation) or UNDER it (a
    // folder's). Collected before any mutation so the relocate loop can re-key
    // safely. PathKey::isAtOrUnder compares whole names, so "notes/a.txt" is
    // not under "notes/a"
    [[nodiscard]] QList<AbstractFileModel*>
    modelsAtOrUnder_(const PathKey& entry) const
    {
        QList<AbstractFileModel*> out{};

        for (auto it = models_.constBegin(); it != models_.constEnd(); ++it) {
            if (it.key().isAtOrUnder(entry)) {
                out << it.value();
            }
        }

        return out;
    }

    // Re-key one buffer after the disk rename. oldBase/newBase are the renamed
    // entry's own keys for a file, or the folder's for a folder rename — in
    // which case the buffer's tail below oldBase is moved under newBase. Moves
    // the map key, the write-fingerprint, and the watch, then updates identity
    // (which fans the retitle out to views).
    //
    // The disk op refuses a target that exists, so a buffer already under the
    // new key is one whose file was deleted outside Fernanda before the
    // watcher's report was reconciled. It's discarded as that reconcile would
    // discard it, so the map never holds two buffers for one key and the
    // moved buffer doesn't take over the other's fingerprint.
    //
    // Each entry is removed before it is put back: a change of letter case
    // alone gives an equal key where case is ignored, and inserting first
    // would leave the old spelling in the map
    void rekeyModel_(
        AbstractFileModel* model,
        const PathKey& oldBase,
        const PathKey& newBase)
    {
        auto old_key = model->key();
        auto new_key = old_key.rebased(oldBase, newBase);

        if (auto* stale = models_.value(new_key); stale && stale != model) {
            discardModel_(stale);
        }

        models_.remove(old_key);
        models_.insert(new_key, model);

        if (lastWrittenHash_.contains(old_key)) {
            auto hash = lastWrittenHash_.take(old_key);
            lastWrittenHash_.insert(new_key, hash);
        }

        // Dropped before the disk op, re-armed here
        watcher_->addPath(new_key.path().toQString());
        model->renameTo(new_key);
    }

    // What writeModel_ did. FileGone isn't a save failure: the file was deleted
    // or moved outside Fernanda — or its folder was — and the watcher hasn't
    // reported it. Callers that report failures leave it out
    enum class WriteResult_
    {
        Written,
        Failed,
        FileGone
    };

    // The single write site. Every path to disk — the debounce, the ceiling,
    // the exit flushes, the eviction flush — funnels through here, so the
    // watcher's self-write suppression records its fingerprint in one place,
    // and a failed write shouts in one place. Leaves the model dirty on
    // failure: a buffer is never abandoned, so the next save trigger retries
    // it.
    //
    // It only ever OVERWRITES. A live buffer's file is always on disk, so a
    // missing file means it was deleted or moved outside Fernanda, its folder
    // included, and the watcher hasn't said so — deleting or renaming a PARENT
    // folder may produce no fileChanged for the file under it. Writing anyway
    // would recreate the file, and with CreateDirs its whole folder chain,
    // somewhere the user just emptied. Instead the path goes to
    // reconcileChanges_ as if the watcher had reported it: its deletion branch
    // discards the buffer and closes the views, matching Obsidian, which sees
    // an outside folder move as a delete per file. Not discarded here: callers
    // are iterating models_. And the debounce lets a brief disappearance (a git
    // checkout deleting and recreating a folder) come back before anything is
    // discarded
    WriteResult_ writeModel_(AbstractFileModel* model)
    {
        auto key = model->key();
        auto abs = key.path();
        auto q_abs = abs.toQString();

        if (!abs.exists()) {
            if (abs.parent().exists()) {
                WARN(
                    this,
                    "Save skipped — {} is gone from disk!",
                    abs.prettyQString());
            } else {
                WARN(
                    this,
                    "Save skipped — the folder holding {} is gone from disk!",
                    abs.prettyQString());
            }

            pendingChanged_ << q_abs;
            watchReconcileTimer_->start(watchReconcileMs_);
            return WriteResult_::FileGone;
        }

        // Capture once: the bytes we write and the bytes we fingerprint must be
        // identical, and data() is a real conversion for text (prime -> UTF-8)
        auto bytes = model->data();

        // Drop the watch across our own write. On Windows QSaveFile's atomic
        // commit-rename can fail with a sharing violation if the watcher engine
        // holds a transient handle, and the rename drops the watch anyway.
        // Re-add after
        watcher_->removePath(q_abs);

        // CreateDirs::No closes the gap between the check above and here
        if (!Io::write(bytes, abs, Io::CreateDirs::No)) {
            // The old file may still be on disk (nothing replaced it), so
            // restore the watch before bailing. Buffer kept dirty
            if (abs.exists()) {
                watcher_->addPath(q_abs);
            }

            CRITICAL(
                this,
                "Save FAILED for {} — buffer kept dirty!",
                abs.prettyQString());
            return WriteResult_::Failed;
        }

        // Fingerprint exactly what we wrote, then re-watch. reconcileChanges_
        // compares disk to this, so our own save — and its watcher echo — is
        // recognized and ignored no matter what the buffer does next
        lastWrittenHash_.insert(key, qHash(bytes));
        watcher_->addPath(q_abs);

        model->setModified(false);
        DEBUG(this, "Save succeeded for {}!", abs.prettyQString());

        emit fileWritten(key);
        return WriteResult_::Written;
    }

    // A buffer's file is gone — deleted externally (reconcileChanges_) or
    // trashed in-app (applyRemoval). One step for both, since they're the
    // same event from different sources. No off-disk buffers, so the buffer
    // follows the file: drop it from the map, stop watching, forget its
    // fingerprint, announce (every view on it self-closes — this window's
    // pop-outs, and every window for a common file), free. Each view's late
    // lastViewClosed then lands on evictModel_'s stale-signal guard
    void discardModel_(AbstractFileModel* model)
    {
        auto key = model->key();

        models_.remove(key);
        watcher_->removePath(key.path().toQString());
        lastWrittenHash_.remove(key);
        model->notifyRemovedFromDisk();
        model->deleteLater();
    }

    // A model's last view closed: flush any unsaved bytes, then free the
    // buffer, so models_ doesn't grow without bound as tabs come and go. The
    // tab-close path lands here — closing a tab destroys the view, the base
    // model's view count returns to zero, and lastViewClosed fires. Tab close
    // itself does no IO; this is the one place a viewless buffer's final
    // per-file flush belongs.
    //
    // SAFETY rests on a destruction-ordering invariant: a view firing this
    // always reaches a LIVE store. The store is App's child and never dies on a
    // window close. At app quit, ~QApplication reaps top-level windows before
    // ~QObject reaps App's children. So the store is alive whenever a view of
    // its models dies, and no closingDown() guard is needed
    void evictModel_(AbstractFileModel* model)
    {
        auto key = model->key();

        // Stale-signal guard. External deletion (reconcileChanges_) already
        // dropped this model from the map and deleteLater'd it; its views then
        // self-close on removedFromDisk, and the LAST one firing lastViewClosed
        // lands here on a model already gone. The identity check additionally
        // refuses to free a DIFFERENT model re-inserted under the same key
        // (delete + recreate + reopen) off an old model's late signal
        auto it = models_.constFind(key);

        if (it == models_.constEnd() || it.value() != model) {
            return;
        }

        // Flush then free. A failed final write KEEPS the viewless dirty model
        // in the map rather than freeing it — a buffer is never abandoned; the
        // exit sweeps and the next close retry it. writeModel_ already logged
        // CRITICAL, so just bail. A file gone from disk keeps it too, briefly:
        // the reconcile pass writeModel_ queued discards it
        if (model->isModified() &&
            writeModel_(model) != WriteResult_::Written) {
            return;
        }

        models_.remove(key);
        watcher_->removePath(key.path().toQString());
        lastWrittenHash_.remove(key);
        model->deleteLater();

        DEBUG(
            this,
            "Evicted viewless model for {} (total: {})",
            key.path(),
            models_.size());
    }

    // Either timer fired: write every dirty buffer, and stop BOTH — whichever
    // fired cancels the other, so a burst produces one write. Failures are
    // logged in writeModel_ and left dirty; the next edit re-arms and retries,
    // and focus-loss/exit are the backstops. No prompt here: the visible prompt
    // lives on close, where it can still refuse
    void autosave_()
    {
        TRACER;

        autosaveDebounceTimer_->stop();
        autosaveCeilingTimer_->stop();

        for (auto* model : models_) {
            if (model->isModified() && !isHeld_(model)) {
                writeModel_(model);
            }
        }
    }

    // Where an entry's folder watches are in one tree: the entry itself, or,
    // when the entry holds that tree's vault folder, the whole tree. A tree
    // with nothing listed there does nothing (VaultTreeModel::releaseWatches)
    [[nodiscard]] static Coco::Path
    watchedPathIn_(const VaultTreeModel* tree, const PathKey& entry)
    {
        auto root = tree->root();

        return PathKey::of(root).isUnder(entry) ? root
                                                : spelledIn_(tree, entry);
    }

    // The entry as one tree spells it: the tree's own root, then the entry's
    // path below that root. A tree finds its rows by comparing names exactly,
    // and where letter case is ignored, two vaults' roots may spell the
    // folders above them differently: one opened as "C:/Users/me/x", another
    // as "c:/users/me/x/y". Below a root, every tree lists the same folders
    // from disk, so the names agree. An entry outside the tree is returned as
    // it is, and the tree finds nothing there
    [[nodiscard]] static Coco::Path
    spelledIn_(const VaultTreeModel* tree, const PathKey& entry)
    {
        auto root = tree->root();
        auto root_key = PathKey::of(root);

        if (entry == root_key) {
            return root;
        }

        if (entry.isUnder(root_key)) {
            return root / entry.relativeTo(root_key);
        }

        return entry.path();
    }

    // Write one buffer if it's modified. A buffer already matching disk is
    // Written, having written nothing
    WriteResult_ flushOne_(AbstractFileModel* model)
    {
        return model->isModified() ? writeModel_(model) : WriteResult_::Written;
    }

    // Every edit re-arms the debounce so the write lands after typing pauses.
    // The ceiling is armed once per burst and never restarted, so it still
    // fires under input that never leaves a gap — Obsidian's hybrid: save when
    // editing stops, but save ANYWAY if it never does
    void onModelContentChanged_()
    {
        autosaveDebounceTimer_->start(autosaveDebounceMs_);

        if (!autosaveCeilingTimer_->isActive()) {
            autosaveCeilingTimer_->start(autosaveCeilingMs_);
        }
    }

    // A watched file changed. Coalesce onto the debounce — our own write's echo
    // included, since reconcileChanges_ recognizes it by fingerprint rather
    // than us trying to guess which event it is. The path is absolute, exactly
    // as we added it
    void onWatchedFileChanged_(const QString& path)
    {
        pendingChanged_ << path;
        watchReconcileTimer_->start(watchReconcileMs_);
    }

    // Process one coalesced burst of external changes. Snapshot and clear
    // first, so events that land while we work re-arm the next pass cleanly
    // rather than being dropped
    void reconcileChanges_()
    {
        TRACER;

        const auto changed = pendingChanged_;
        pendingChanged_.clear();

        for (const auto& changed_path : changed) {
            reconcileOne_(changed_path);
        }
    }

    // One file the watcher reported, or a held file at its release: take in
    // what's on disk if it differs from what this store last wrote there
    void reconcileOne_(const Coco::Path& path)
    {
        auto key = PathKey::of(path);

        auto it = models_.constFind(key);

        if (it == models_.constEnd()) {
            return; // not a file we hold a buffer for
        }

        auto* model = it.value();

        // External deletion: the buffer follows the file, through the same
        // discard an in-app trash uses
        if (!path.exists()) {
            DEBUG(
                this,
                "External deletion: {} — closing views",
                path.prettyQString());
            discardModel_(model);
            return;
        }

        // The file exists and changed. Read it — but never ACT on a failed
        // read (a file locked mid-write by the external tool): treating
        // that as "now empty" could clobber the buffer. Re-add the watch (a
        // rename may have dropped it) and defer to the next pass, when the
        // tool's final rename will have fired another event
        auto disk = Io::tryRead(path);

        if (!disk) {
            watcher_->addPath(path.toQString());
            pendingChanged_ << path.toQString();
            watchReconcileTimer_->start(watchReconcileMs_);
            return;
        }

        // An external editor's atomic-rename save can also drop our watch;
        // re-add before any branch below returns
        watcher_->addPath(path.toQString());

        auto disk_hash = qHash(*disk);

        // Our own write: disk still matches the fingerprint writeModel_
        // recorded. True regardless of how far the buffer has advanced —
        // which is why we compare to the fingerprint and not to the live
        // buffer. Ignore it
        if (auto h = lastWrittenHash_.constFind(key);
            h != lastWrittenHash_.constEnd() && *h == disk_hash) {
            return;
        }

        // A genuine external change. If the tool wrote exactly what we
        // already hold, there is nothing to do — sync our fingerprint and
        // move on, so a no-op change is silent
        if (*disk == model->data()) {
            lastWrittenHash_.insert(key, disk_hash);
            DEBUG(
                this,
                "External no-op (disk == buffer): {}",
                path.prettyQString());
            return;
        }

        // An external tool re-encoded the file out of UTF-8. There's no GUI
        // here to ask, so it's adopted like any external change: the
        // undecodable sequences become U+FFFD in the buffer and reach disk
        // at the next edit. Rare, and accepted — logged so it's findable
        // when someone asks where their quotes went
        if (FileTypes::typeOf(path) == FileTypes::Type::Text &&
            !TextFileModel::isValidUtf8(*disk)) {
            WARN(
                this,
                "External change isn't valid UTF-8 — adopting lossily: {}",
                path.prettyQString());
        }

        // Adopt the external change. reloadContent routes through the prime
        // as an UNDOABLE replace, so Ctrl+Z reverts it and the next
        // autosave rewrites the buffer — the escape hatch that lets us
        // adopt unconditionally rather than prompting. A dirty buffer's
        // unsaved edits are exactly what undo restores, so nothing is lost
        // that the user can't get back. (If a future case ever needs to
        // BLOCK an external rewrite — a merge in progress, a protected file
        // — the gate goes here, on model->isModified().)
        DEBUG(this, "External change — reload: {}", path.prettyQString());
        model->reloadContent(*disk);
        lastWrittenHash_.insert(key, disk_hash);
        model->notifyReloaded();
    }
};

} // namespace Fernanda
