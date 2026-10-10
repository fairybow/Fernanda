/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>
#include <type_traits>

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/FileRef.h"
#include "core/FileTypes.h"
#include "core/Io.h"
#include "core/VaultConfig.h"
#include "core/VaultDotDir.h"
#include "core/VaultTreeModel.h"
#include "core/spell/WordList.h"
#include "models/AbstractFileModel.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"

namespace Suzuri {

using namespace Qt::StringLiterals;

// The GUI-free model side of a vault: a root directory, a map of open-file
// buffers, a watcher over those files, and the file tree every view of this
// vault shares. No GUI types. Owned by App; borrowed by a VaultWindow.
//
// The Vault owns all disk IO. Models are pure buffers: the Vault reads bytes
// and hands them to setData() on open, and reads data() back to write on flush.
// One place writes, so one place suppresses the watcher's self-write.
//
// See docs/Architecture.md, "Saving" and "Watching the filesystem".
class Vault : public QObject
{
    Q_OBJECT

public:
    Vault(const Coco::Path& root, QObject* parentApp)
        : QObject(parentApp)
        , root_(root)
    {
        setup_();
    }

    ~Vault() override { TRACER; }

    [[nodiscard]] Coco::Path root() const noexcept { return root_; }

    // The vault's file tree, shared by every tree view of this vault in every
    // window. Views borrow it; the Vault owns it
    [[nodiscard]] VaultTreeModel* treeModel() const noexcept
    {
        return treeModel_;
    }

    // Absolute <-> vault-relative. The map and FileRefs are relative; disk IO
    // is absolute. Callers with an absolute path (a tree click, an open dialog)
    // go through makeFileRef to enter the relative world
    [[nodiscard]] Coco::Path absolutePathOf(const Coco::Path& relative) const
    {
        return root_ / relative;
    }

    [[nodiscard]] Coco::Path relativePathOf(const Coco::Path& absolute) const
    {
        return absolute.lexicallyRelative(root_);
    }

    [[nodiscard]] FileRef makeFileRef(const Coco::Path& absolute)
    {
        return { this, relativePathOf(absolute) };
    }

    // Does an absolute path belong to this vault — is it root or nested under
    // it? Routes a file dropped on an editor to the owning vault. Rejects a
    // path outside the root, where relativePathOf would produce a "../"-laden
    // key, so a drag from another window's vault opens nothing here.
    //
    // The path must also be plain (Coco::Path::isPlain). isAtOrUnder compares
    // the path as written, and "<root>/a/../.." is written under the root it
    // climbs out of; a plain path has no such segment, so written under the
    // root means under the root. A second spelling of an inside path
    // ("<root>/a/./b") is refused as well, never normalized: its relative form
    // would be a second key for one file.
    //
    // Every public function that takes an absolute path answers to this
    [[nodiscard]] bool contains(const Coco::Path& absolute) const
    {
        return absolute.isPlain() && absolute.isAtOrUnder(root_);
    }

    // Every file this vault shows, vault-relative — the FileSwitcher's listing.
    // Walked fresh on each call: no index and no watcher, so the list is a
    // snapshot of the moment it's asked for. Agrees with the trees by
    // construction — the same two predicates VaultTreeModel lists by. A hidden
    // folder (.suzuri/, .git/) is pruned, never walked; a file must be neither
    // hidden nor unsupported. Unsorted
    [[nodiscard]] Coco::PathList visibleFiles() const
    {
        auto absolutes =
            Coco::walkFilePaths(root_, [](const Coco::Path& subdir) {
                return !FileTypes::isHiddenName(subdir.nameQString());
            });

        Coco::PathList result{};

        for (const auto& absolute : absolutes) {
            if (FileTypes::isHiddenName(absolute.nameQString())) {
                continue;
            }

            if (!FileTypes::isSupported(absolute)) {
                continue;
            }

            result << relativePathOf(absolute);
        }

        return result;
    }

    // Asked by openModel when a text file doesn't decode cleanly as UTF-8: true
    // opens it anyway, false refuses the open. Given the vault-relative path
    // and the raw bytes, so a GUI caller can name the file and preview the
    // damage. A plain callable keeps the Vault GUI-free — the window supplies
    // the prompt
    using ConfirmLossyOpen =
        std::function<bool(const Coco::Path& relative, const QByteArray& data)>;

    // Return the existing buffer for this file, or create one. This is the
    // dedup point that makes "same file open twice" and "common file open in
    // two windows" both resolve to a single model.
    //
    // Returns nullptr for a file Suzuri won't open: a path that isn't a
    // vault-relative key (see isKey_), an unsupported type, a file it can't
    // read — gone since it was listed, or locked — or a text file that
    // isn't valid UTF-8 whose open the caller declined. This is the
    // AUTHORITATIVE refusal: the tree and the FileSwitcher hide unsupported
    // files, but those are advisory, and this is the one gate every open path —
    // tree, switcher, drop, restore, and later CLI — funnels through. Callers
    // treat null as "nothing to open".
    //
    // confirmLossyOpen is asked only on a fresh read that fails to decode — a
    // dedup hit returns above it, so an open buffer never prompts twice (a
    // common file opened from a second window included)
    [[nodiscard]] AbstractFileModel* openModel(
        const Coco::Path& relative,
        const ConfirmLossyOpen& confirmLossyOpen)
    {
        // Before anything else: a key that isn't one can't be in the map, and
        // absolutePathOf would turn it into a path outside the vault
        if (!isKey_(relative)) {
            WARN(
                "Refusing to open {}: not a path inside the vault!",
                relative.prettyQString());
            return nullptr;
        }

        if (auto it = models_.constFind(relative); it != models_.constEnd()) {
            return it.value();
        }

        // Refuse BEFORE the read: an unsupported file may be large (a video)
        // and there is no reason to pull it into memory just to discard it
        auto type = FileTypes::typeOf(relative);
        if (type == FileTypes::Type::Unsupported) {
            WARN(
                "Refusing to open unsupported file type: {}",
                relative.prettyQString());
            return nullptr;
        }

        // Refuse what can't be read — deleted since it was listed (a switcher
        // snapshot, a tree click racing a delete) or locked by another
        // program. A buffer built over nothing opens empty, and the first
        // keystroke would autosave it back into existence, or over the locked
        // original once the lock clears. tryRead, unlike read, tells an empty
        // file from a failed read, and has already logged which failure
        auto data = Io::tryRead(absolutePathOf(relative));
        if (!data) {
            WARN(
                "Refusing to open unreadable file: {}",
                relative.prettyQString());
            return nullptr;
        }

        // Text that isn't valid UTF-8 — usually an older encoding like
        // Windows-1252, or a binary under a text extension. Suzuri does no
        // conversion, so the caller decides: open anyway, or refuse. Asked
        // before a model exists, so a declined file never becomes a buffer
        auto decodes_lossily =
            type == FileTypes::Type::Text && !TextFileModel::isValidUtf8(*data);

        if (decodes_lossily && !confirmLossyOpen(relative, *data)) {
            INFO(
                "Declined opening non-UTF-8 file: {}",
                relative.prettyQString());
            return nullptr;
        }

        // Dispatch by file type. The dedup above means one buffer per file
        // regardless of type
        FileRef ref{ this, relative };
        auto* model = makeModel_(ref, type);

        model->setData(*data);
        models_.insert(relative, model);

        // Start watching now that a buffer exists. Removed again on eviction or
        // external deletion; dropped and re-added around our own writes in
        // writeModel_ so a save isn't read back as an external change
        watcher_->addPath(absolutePathOf(relative).toQString());

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
            &Vault::onModelContentChanged_);

        // Last view closed → flush and free the buffer. The base model counts
        // its own views (GUI-free) and fires this on the 1->0 edge; the lambda
        // re-derives the map key inside evictModel_
        connect(model, &AbstractFileModel::lastViewClosed, this, [this, model] {
            evictModel_(model);
        });

        DEBUG(
            this,
            "Opened model for {} (total: {})",
            relative,
            models_.size());

        return model;
    }

    // Write every modified buffer back to disk. Returns the vault-relative
    // paths that FAILED to write (empty == all good; a buffer whose file is
    // gone from disk doesn't count), so a caller — VaultWindow::closeEvent —
    // can name them in the one surviving prompt and refuse the close.
    //
    // Synchronous, and skips buffers already matching disk. Used by the on-exit
    // flushes and App's focus-loss flush; the while-typing autosave uses
    // autosave_ below. Deliberately does NOT touch the autosave timers — this
    // is pure buffer IO. A pending settings save is the exception: it is
    // written now rather than left to a timer that may never fire, since every
    // exit path runs through here. Its failure isn't reported — a settings file
    // is never worth refusing a close over, and Io logged it
    Coco::PathList flush()
    {
        if (configSaveTimer_->isActive()) {
            saveConfig_();
        }

        Coco::PathList failed{};

        for (auto* model : models_) {
            if (!model->isModified()) {
                continue;
            }

            // A file gone from disk isn't a save failure — there's nowhere left
            // to save it, and the reconcile pass writeModel_ queued is about to
            // close its views — so it isn't reported
            if (writeModel_(model) == WriteResult_::Failed) {
                failed << model->fileRef().relative;
            }
        }

        return failed;
    }

    // Create a new empty file / new folder inside an absolute directory (a
    // vault folder, or the vault root) and return its vault-relative path
    // (empty on failure). Callers hand an absolute path — a tree click or the
    // new-tab page's root — and get back the relative key the model map and
    // FileRefs speak in. All disk IO is the Vault's; there are no off-disk
    // buffers, so the file exists on disk before any buffer opens it
    Coco::Path createFile(const Coco::Path& absoluteDir)
    {
        if (!contains(absoluteDir)) {
            WARN("Can't create a file in {}: outside the vault!", absoluteDir);
            return {};
        }

        auto absolute = uniqueChildPath_(absoluteDir, u"Untitled"_s, u".txt"_s);

        // CreateDirs::No: a folder deleted between the right-click and the
        // click stays deleted, and the create fails
        if (!Io::write(QByteArray{}, absolute, Io::CreateDirs::No)) {
            WARN("Failed to create a new file at {}!", absolute);
            return {};
        }

        treeModel_->applyAddition(absolute);
        return relativePathOf(absolute);
    }

    Coco::Path createFolder(const Coco::Path& absoluteDir)
    {
        if (!contains(absoluteDir)) {
            WARN(
                "Can't create a folder in {}: outside the vault!",
                absoluteDir);
            return {};
        }

        auto absolute = uniqueChildPath_(absoluteDir, u"Untitled"_s, QString{});

        if (!Coco::mkdir(absolute)) {
            WARN("Failed to create a new folder at {}!", absolute);
            return {};
        }

        treeModel_->applyAddition(absolute);
        return relativePathOf(absolute);
    }

    // Create an empty file at a vault-relative path named in full, making any
    // missing folders above it. This is Go to File's create: the user typed
    // the path, so the folders in it are theirs to make, where createFile's
    // folder must already be there. The vault's own folder is never recreated.
    //
    // Returns false, having made nothing, for a path that isn't a key (see
    // isKey_), has a hidden name anywhere along it, isn't a text file (an empty
    // PDF or image is no file at all), or already exists. Also false when the
    // disk refuses, as when a part of the path is a file; folders made before
    // the refusal stay, and the tree finds them by its watcher.
    //
    // The tree is told about the highest entry made, so a new folder appears
    // with the file inside it. The tree can't place the file itself while the
    // folder holding it has no row
    bool createFileAt(const Coco::Path& relative)
    {
        if (!isKey_(relative)) {
            WARN(
                "Can't create {}: not a path inside the vault!",
                relative.prettyQString());
            return false;
        }

        for (auto part = relative; !part.isEmpty(); part = part.parent()) {
            if (FileTypes::isHiddenName(part.nameQString())) {
                WARN(
                    "Can't create {}: a name begins with a period!",
                    relative.prettyQString());
                return false;
            }
        }

        if (FileTypes::typeOf(relative) != FileTypes::Type::Text) {
            WARN("Can't create {}: not a text file!", relative.prettyQString());
            return false;
        }

        if (!root_.isDir()) {
            WARN(
                "Can't create {}: the vault folder is gone!",
                relative.prettyQString());
            return false;
        }

        auto absolute = absolutePathOf(relative);

        if (absolute.exists()) {
            WARN("Can't create {}: it already exists!", absolute);
            return false;
        }

        auto highest = absolute;

        for (auto folder = absolute.parent();
             folder.isUnder(root_) && !folder.exists();
             folder = folder.parent()) {
            highest = folder;
        }

        if (!Io::write(QByteArray{}, absolute, Io::CreateDirs::Yes)) {
            WARN("Failed to create a new file at {}!", absolute);
            return false;
        }

        treeModel_->applyAddition(highest);
        return true;
    }

    // Rename a file or folder in place — same parent, new leaf name.
    // absoluteOld is an existing entry in this vault; newLeaf is its new
    // on-disk name (a file's extension is already reattached by the caller —
    // the tree hides it). Returns the new vault-relative path (empty on
    // failure). A rename is a relocation whose parent doesn't change, so it
    // funnels through relocate_ (below) exactly as a move does — the disk op,
    // watcher suppression, and buffer re-keying are shared. Declines cleanly on
    // a locked entry, never corrupts.
    //
    // newLeaf must be one name. Anything with a separator, a "." or "..", or a
    // root would make this a move, and is refused
    Coco::Path rename(const Coco::Path& absoluteOld, const QString& newLeaf)
    {
        auto leaf = Coco::Path(newLeaf);

        if (leaf.isEmpty() || !leaf.isPlain() || leaf.name() != leaf) {
            WARN(
                "Can't rename {} to {}: not a single name!",
                absoluteOld,
                leaf);
            return {};
        }

        return relocate_(absoluteOld, absoluteOld.parent() / newLeaf);
    }

    // Move a file or folder into a different directory. A move is a rename with
    // a new parent, so it shares relocate_'s core: the leaf name is preserved,
    // so the new path is absoluteDestDir/name. absoluteEntry is an existing
    // entry in this vault; absoluteDestDir is where it lands (a tree folder, or
    // the vault root for a drop on empty space). Any open buffer at or under
    // the entry is re-keyed just as a folder rename does. Returns the new
    // vault-relative path (empty on failure — a name collision, a locked entry,
    // or an invalid target). Declines cleanly, never corrupts.
    //
    // The guards below are defensive, behind the tree's advisory ones: the tree
    // suppresses these drops (no indicator, ignored), but the disk op stays
    // authoritative, so a race or a future non-tree caller can't drive a
    // corrupting move through. Vault scope is relocate_'s to check, for rename
    // and move alike. The no-op case (dropping an entry into its own current
    // parent) is the tree's to filter — it's a pure path comparison needing no
    // Vault, and letting it reach here would only turn a harmless nothing into
    // a spurious failure warning
    Coco::Path
    move(const Coco::Path& absoluteEntry, const Coco::Path& absoluteDestDir)
    {
        if (!absoluteDestDir.isDir()) {
            WARN("Move target {} is not a directory!", absoluteDestDir);
            return {};
        }

        // Nothing can move into itself or its own descendant. isAtOrUnder walks
        // parents component-wise (no false prefix hit) and covers the
        // entry-onto-itself case in the same call. Only folders can fail this:
        // the destination is a directory, and a directory is never at or under
        // a file
        if (absoluteDestDir.isAtOrUnder(absoluteEntry)) {
            WARN("Can't move {} into itself or a descendant!", absoluteEntry);
            return {};
        }

        return relocate_(absoluteEntry, absoluteDestDir / absoluteEntry.name());
    }

    // Move a file or folder to the system trash. absoluteEntry is an existing
    // entry in this vault — never the root. Every open buffer at or under it is
    // flushed first, so the trashed copy holds the latest text, then discarded
    // once the entry is gone: the same step the watcher's deletion branch
    // takes, so every view on those buffers — every window's, for a common file
    // — closes.
    //
    // Returns false with nothing changed on disk or in any buffer. One bool,
    // but each refusal logs its own reason, so the log says which it was: out
    // of scope, the root, already gone, a failed flush (naming the file), or
    // the system trash refusing. There is no local-trash fallback
    bool moveToTrash(const Coco::Path& absoluteEntry)
    {
        // Same authority relocate_ holds: relativePathOf would answer a
        // "../"-laden path for an outside entry, and modelsAtOrUnder_ would
        // then match nothing and the disk op would still run. contains also
        // refuses every other spelling of the root ("<root>/a/..", "<root>/"),
        // which the comparison below would not recognize
        if (!contains(absoluteEntry)) {
            WARN("Trash refused — {} is outside the vault!", absoluteEntry);
            return false;
        }

        // The tree never offers the root, but this is the one place a future
        // caller has to be stopped from trashing the whole vault
        if (absoluteEntry == root_) {
            WARN("Trash refused — {} is the vault root!", absoluteEntry);
            return false;
        }

        // Named here rather than left to read as the system trash refusing.
        // If it held open buffers, the watcher's deletion branch handles them
        if (!absoluteEntry.exists()) {
            WARN("Trash refused — {} no longer exists!", absoluteEntry);
            return false;
        }

        auto relative = relativePathOf(absoluteEntry);
        auto affected = modelsAtOrUnder_(relative);

        // Flush BEFORE unwatching: writeModel_ drops and re-adds its own watch
        // around the write, which would undo an earlier unwatch. A viewless
        // dirty model (a failed eviction flush) is in models_ too, so it gets
        // its retry here. Any failure refuses the whole delete before disk is
        // touched — trashing a file whose buffer never reached it would lose
        // that text. writeModel_ has already logged CRITICAL. A buffer whose
        // file is already gone has nothing to trash and doesn't refuse; it's
        // discarded with the rest below
        for (auto* model : affected) {
            if (model->isModified() &&
                writeModel_(model) == WriteResult_::Failed) {
                WARN(
                    "Trash refused — {} couldn't be saved first: {}",
                    absoluteEntry,
                    model->fileRef().relative);
                return false;
            }
        }

        // Unwatch before the disk op, as relocate_ does, so the entry's
        // disappearance isn't queued as an external deletion. (A flush's own
        // watcher echo may still be queued; by the time it's reconciled the
        // buffer is gone from models_, so it's skipped.) The tree's folder
        // watches inside the entry go too: on Windows each is a directory
        // handle, and any handle inside a folder stops the trash taking it
        for (auto* model : affected) {
            watcher_->removePath(
                absolutePathOf(model->fileRef().relative).toQString());
        }

        treeModel_->releaseWatches(absoluteEntry);

        if (!Coco::moveToTrash(absoluteEntry)) {
            for (auto* model : affected) {
                watcher_->addPath(
                    absolutePathOf(model->fileRef().relative).toQString());
            }

            treeModel_->restoreWatches(absoluteEntry);

            WARN(
                "Trash refused — the system trash wouldn't take {}!",
                absoluteEntry);
            return false;
        }

        // No event-loop turn between the disk op and here, so no autosave tick
        // can write a buffer back into the gap and resurrect the entry
        treeModel_->applyRemoval(absoluteEntry);

        for (auto* model : affected) {
            discardModel_(model);
        }

        DEBUG(
            this,
            "Moved {} to the system trash ({} open buffer(s) discarded)",
            relative,
            affected.size());

        return true;
    }

    // The vault's folder was deleted or moved outside Suzuri: make it again,
    // empty, in place. Only App calls this, and only for the Common Vault — a
    // standing folder, rebuilt at startup anyway — while a project vault whose
    // folder vanishes has its window closed instead. The same Vault object
    // stays, since every project window holds a pointer to it. Every open
    // buffer is also handed to the reconcile: their files went with the folder,
    // and a clean buffer the watcher missed would otherwise sit open until
    // typed into. Returns false if the folder can't be made
    bool recreateRoot()
    {
        if (!root_.isDir() && !Coco::mkpath(root_)) {
            CRITICAL(this, "Couldn't recreate vault folder {}!", root_);
            return false;
        }

        ensureVaultDotDir(root_);
        treeModel_->relistRoot();

        for (auto* model : models_) {
            auto abs = absolutePathOf(model->fileRef().relative);
            pendingChanged_ << abs.toQString();
        }

        if (!pendingChanged_.isEmpty()) {
            watchReconcileTimer_->start(watchReconcileMs_);
        }

        INFO(this, "Recreated vault folder {}", root_);
        return true;
    }

    // Adjust the while-typing autosave: the debounce (write this long after
    // typing pauses) and the ceiling (write ANYWAY this long into unbroken
    // input). Not a saved setting yet
    void setAutosaveTiming(int debounceMs, int ceilingMs)
    {
        autosaveDebounceMs_ = debounceMs;
        autosaveCeilingMs_ = ceilingMs;
    }

    // --- Per-vault settings --------------------------------------------------

    // The vault's config, loaded at construction. Read-only here: every change
    // goes through setConfig below, so each one is announced and saved and
    // none can slip past both
    [[nodiscard]] const VaultConfig& config() const noexcept { return config_; }

    // Change one setting: setter is the VaultConfig setter to call, value what
    // to give it. The one way a setting changes, so each change is announced
    // and saved. Only a real change (VaultConfig's setters report it) announces
    // configChanged and arms the save, so re-picking the current font writes
    // nothing.
    //
    // std::type_identity_t keeps value out of deduction: ValueT comes from the
    // setter alone, and value converts to it
    template <typename ValueT>
    void setConfig(
        bool (VaultConfig::*setter)(ValueT),
        std::type_identity_t<ValueT> value)
    {
        if ((config_.*setter)(value)) {
            onConfigChanged_();
        }
    }

    // --- Spelling ------------------------------------------------------------

    // The vault's own dictionary: words to take as correctly spelled in this
    // vault, kept in .suzuri/dictionary.txt and committed with the vault. Read
    // at construction; a change made to the file by hand shows the next time
    // the vault opens, or at the next addToDictionary
    [[nodiscard]] const WordList& dictionary() const noexcept
    {
        return dictionary_;
    }

    // Words to take as correctly spelled until the vault closes. Never saved
    [[nodiscard]] const WordList& ignoredWords() const noexcept
    {
        return ignoredWords_;
    }

    // Add a word to the dictionary and its file. The file is read again first
    // and becomes the dictionary with the word added, so a word added or
    // removed by hand since the vault opened is kept as the file has it. A file
    // that is there but can't be read is not written over: the word is added
    // for this session only. Announces wordsChanged
    void addToDictionary(const QString& word)
    {
        auto path = dictionaryPath_();
        auto on_disk = WordList::read(path);

        if (!on_disk) {
            WARN("Couldn't read {}; adding the word for now only", path);

            if (dictionary_.insert(word)) {
                emit wordsChanged();
            }

            return;
        }

        if (on_disk->insert(word) && ensureVaultDotDir(root_)) {
            on_disk->write(path);
        }

        if (*on_disk != dictionary_) {
            dictionary_ = *on_disk;
            emit wordsChanged();
        }
    }

    void ignoreWord(const QString& word)
    {
        if (ignoredWords_.insert(word)) {
            emit wordsChanged();
        }
    }

signals:
    // The config changed in memory. Every view this vault's windows host
    // re-applies from config() — VaultWindow::makeView_ connects each one — and
    // so does each window's status bar (BaseWindow::applyConfig). Fires before
    // the (debounced) save, so the change shows at once
    void configChanged();

    // The dictionary or the ignored words changed. Each text view this vault's
    // windows host takes the words again — VaultWindow::makeView_ connects
    // each one — and so does every text view of every vault when the changed
    // vault is the Common Vault
    void wordsChanged();

private:
    Coco::Path root_;

    // Per-vault settings, read at construction and saved a beat after the last
    // change: a dragged size slider changes it many times a second, and one
    // write per settle is enough. A pending save is also forced by flush(), so
    // every exit path that saves buffers saves this too
    VaultConfig config_{};
    Coco::Time::Debouncer* configSaveTimer_ =
        Coco::Time::newDebouncer(this, &Vault::saveConfig_);
    int configSaveMs_ = 500;

    static inline const QString DICTIONARY_FILE_NAME_ = u"dictionary.txt"_s;

    WordList dictionary_{};
    WordList ignoredWords_{};

    // Lists and watches the folders the tree views have opened. A second
    // watcher beside watcher_ below, by design: that one tracks the files we
    // hold buffers for, this one the folders the trees show
    VaultTreeModel* treeModel_ = new VaultTreeModel(root_, this);

    // Keyed by vault-relative path. Models are parented to the Vault. A model
    // is freed when its file is deleted on disk (external deletion, below) or
    // trashed in-app (moveToTrash) — both through discardModel_ — or when its
    // last view closes (eviction — evictModel_); otherwise it lives for the
    // Vault's lifetime
    QHash<Coco::Path, AbstractFileModel*> models_{};

    // Watches the absolute path of every open file. Only files we hold buffers
    // for — the tree's folders are treeModel_'s own watcher's concern
    QFileSystemWatcher* watcher_ = new QFileSystemWatcher(this);

    // Self-write suppression. writeModel_ records a hash of the bytes it wrote
    // here, keyed by absolute path. The reconcile pass compares disk against
    // THIS fingerprint, not the live buffer — so our own save is recognized
    // however far typing has since moved the buffer on, and a genuine later
    // change (different bytes, different hash) is never mistaken for it. A
    // one-shot "ignore the next event" flag would not do: on a platform that
    // sends no echo it lingers and eats the next real change, where a
    // fingerprint can't be "used up". Same strings we pass to addPath, so the
    // keys match. Entries are dropped when a buffer is discarded or evicted
    QHash<QString, size_t> lastWrittenHash_{};

    // Watcher events are coalesced here and processed one debounce later, so a
    // git checkout or Dropbox sync rewriting many watched files becomes one
    // reconcile pass, not N — and one collapse sweep, not many. Absolute paths,
    // as QFileSystemWatcher reports them
    QSet<QString> pendingChanged_{};
    Coco::Time::Debouncer* watchReconcileTimer_ =
        Coco::Time::newDebouncer(this, &Vault::reconcileChanges_);

    // Long enough to swallow a burst of filesystem events, short enough to feel
    // immediate
    int watchReconcileMs_ = 250;

    // Hybrid autosave. The debounce restarts on every edit and fires on the
    // pause; the ceiling is armed once per burst and never restarted, so it
    // still fires under input that never pauses — a held key. Both single-shot,
    // both call autosave_, which stops both — so whichever fires first wins the
    // burst
    Coco::Time::Debouncer* autosaveDebounceTimer_ =
        Coco::Time::newDebouncer(this, &Vault::autosave_);
    Coco::Time::Debouncer* autosaveCeilingTimer_ =
        Coco::Time::newDebouncer(this, &Vault::autosave_);

    // The numbers aren't settled. 1000 ms coalesces a typing burst; the 3000 ms
    // ceiling bounds worst-case unsaved time under unbroken input. A writer's
    // inter-keystroke gaps are usually under the debounce, so the ceiling — not
    // the debounce — sets the save cadence during active writing; tune the
    // ceiling to change that
    int autosaveDebounceMs_ = 1000;
    int autosaveCeilingMs_ = 3000;

    // Is this a vault-relative key — the form models_ and FileRefs hold? It
    // names something, has no root (appending a rooted path to root_ replaces
    // part of root_ rather than nesting under it), and is plain, so it stays
    // under the root and is the only spelling of its file
    [[nodiscard]] bool isKey_(const Coco::Path& relative) const
    {
        return !relative.isEmpty() && !relative.hasRoot() && relative.isPlain();
    }

    void setup_()
    {
        ensureVaultDotDir(root_);
        config_.load(root_);
        dictionary_ = WordList::read(dictionaryPath_()).value_or(WordList{});

        // The watcher covers only files we hold buffers for; the tree's folders
        // are treeModel_'s. fileChanged drives external reload and deletion —
        // coalesced, since one git checkout can fire many at once
        connect(
            watcher_,
            &QFileSystemWatcher::fileChanged,
            this,
            &Vault::onWatchedFileChanged_);
    }

    [[nodiscard]] Coco::Path dictionaryPath_() const
    {
        return vaultDotDir(root_) / DICTIONARY_FILE_NAME_;
    }

    // "Untitled.txt", then "Untitled 1.txt", ... — the first not already
    // present inside absoluteDir. An empty ext gives a folder name. Absolute
    // in, absolute out; the caller relativizes. Deliberately not translated:
    // this is on-disk identity, and the user renames it from the tree
    Coco::Path uniqueChildPath_(
        const Coco::Path& absoluteDir,
        const QString& base,
        const QString& ext) const
    {
        auto candidate = absoluteDir / (base + ext);
        auto n = 0;

        while (candidate.exists()) {
            candidate = absoluteDir / (base + " " + QString::number(++n) + ext);
        }

        return candidate;
    }

    // The concrete-model factory. The type is decided by FileTypes::typeOf —
    // extension-based, like Obsidian's own dispatch; no IO decides it, and
    // magic-byte sniffing buys little for the types we open. There is no
    // fallthrough to text: openModel has already refused Unsupported, so
    // reaching that case here is a bug
    [[nodiscard]] AbstractFileModel*
    makeModel_(const FileRef& ref, FileTypes::Type type)
    {
        switch (type) {
        case FileTypes::Type::Text:
            return new TextFileModel(ref, this);
        case FileTypes::Type::Pdf:
            return new PdfFileModel(ref, this);
        case FileTypes::Type::Image:
            return new ImageFileModel(ref, this);
        case FileTypes::Type::Unsupported:
            break;
        }

        ASSERT(false, "makeModel_ reached with an unsupported type!");
        return nullptr;
    }

    // Open buffers whose file is AT rel (a file relocation) or UNDER it (a
    // folder's). Collected before any mutation so the relocate loop can re-key
    // safely. Coco::Path::isAtOrUnder walks parents component-wise, so
    // "notes/a.txt" is not "under" "notes/a" by string prefix
    [[nodiscard]] QList<AbstractFileModel*>
    modelsAtOrUnder_(const Coco::Path& rel) const
    {
        QList<AbstractFileModel*> out{};

        for (auto it = models_.constBegin(); it != models_.constEnd(); ++it) {
            if (it.key().isAtOrUnder(rel)) {
                out << it.value();
            }
        }

        return out;
    }

    // The shared core of rename (in-place) and move (new parent): both relocate
    // an entry from absoluteOld to absoluteNew, and everything past the target
    // path is identical. Suppress the watcher on every buffer AT or UNDER the
    // old path BEFORE the disk op, or the disappearance of `old` reports as an
    // external deletion and closes its views (as writeModel_ suppresses around
    // a write). Snapshot that set now — reused to re-key after, or to restore
    // watches on a failed disk op, where nothing else changes. Returns the new
    // vault-relative path (empty on failure)
    Coco::Path
    relocate_(const Coco::Path& absoluteOld, const Coco::Path& absoluteNew)
    {
        // Both ends must be inside this vault. relativePathOf is
        // lexicallyRelative, which answers "../../elsewhere/note.txt" for a
        // path outside the root rather than failing — and that string would go
        // on to become a models_ key, a watched path, and the FileRef
        // workspace.json persists. The tree already refuses these, but this is
        // the authoritative refusal the callers are entitled to assume
        if (!contains(absoluteOld) || !contains(absoluteNew)) {
            WARN(
                "Relocation of {} to {} leaves the vault!",
                absoluteOld,
                absoluteNew);
            return {};
        }

        auto rel_old = relativePathOf(absoluteOld);
        auto rel_new = relativePathOf(absoluteNew);

        // The tree's folder watches inside the entry are released with the
        // buffers': on Windows each is a directory handle, and any handle
        // inside a folder stops it being renamed or moved
        auto affected = modelsAtOrUnder_(rel_old);

        for (auto* model : affected) {
            watcher_->removePath(
                absolutePathOf(model->fileRef().relative).toQString());
        }

        treeModel_->releaseWatches(absoluteOld);

        if (!renameOnDisk_(absoluteOld, absoluteNew)) {
            for (auto* model : affected) {
                watcher_->addPath(
                    absolutePathOf(model->fileRef().relative).toQString());
            }

            treeModel_->restoreWatches(absoluteOld);

            WARN("Failed to relocate {} to {}!", absoluteOld, absoluteNew);
            return {};
        }

        // The tree first, so views already show the new place when the
        // buffers' retitles fan out
        treeModel_->applyRelocation(absoluteOld, absoluteNew);

        for (auto* model : affected) {
            rekeyModel_(model, rel_old, rel_new);
        }

        DEBUG(this, "Relocated {} -> {}", rel_old, rel_new);
        return rel_new;
    }

    // The disk rename, with the case-only special case. A pure case change
    // ("foo" -> "Foo") on a case-insensitive filesystem is refused by
    // QFile::rename, which sees the target already existing (it's the same
    // file), so route it through a temporary name. Best-effort restore if the
    // second leg fails, so a half-rename never strands the file at the temp
    // name
    [[nodiscard]] bool
    renameOnDisk_(const Coco::Path& oldPath, const Coco::Path& newPath) const
    {
        auto old_name = oldPath.nameQString();
        auto new_name = newPath.nameQString();

        if (old_name != new_name &&
            old_name.compare(new_name, Qt::CaseInsensitive) == 0) {
            auto temp = uniqueSiblingTemp_(oldPath);

            if (!Coco::rename(oldPath, temp)) {
                return false;
            }

            if (!Coco::rename(temp, newPath)) {
                Coco::rename(temp, oldPath); // best-effort restore
                return false;
            }

            return true;
        }

        return Coco::rename(oldPath, newPath);
    }

    // A path in `sibling`'s directory guaranteed not to exist, for the
    // case-only rename's intermediate step
    [[nodiscard]] Coco::Path uniqueSiblingTemp_(const Coco::Path& sibling) const
    {
        auto dir = sibling.parent();
        auto base = sibling.nameQString() + u".suzuri-rename"_s;

        auto candidate = dir / base;
        auto n = 0;

        while (candidate.exists()) {
            candidate = dir / (base + QString::number(++n));
        }

        return candidate;
    }

    // Re-key one buffer from the old path to the new after the disk rename.
    // relOld/relNew are the renamed entry's own paths for a file, or the
    // folder's for a folder rename — in which case the buffer's tail below
    // relOld is transplanted under relNew. Moves the map key, the
    // write-fingerprint, and the watch, then updates identity (which fans the
    // retitle out to views)
    void rekeyModel_(
        AbstractFileModel* model,
        const Coco::Path& relOld,
        const Coco::Path& relNew)
    {
        auto old_rel = model->fileRef().relative;
        auto new_rel = (old_rel == relOld)
                           ? relNew
                           : relNew / old_rel.lexicallyRelative(relOld);

        auto old_abs = absolutePathOf(old_rel).toQString();
        auto new_abs = absolutePathOf(new_rel).toQString();

        models_.remove(old_rel);
        models_.insert(new_rel, model);

        if (auto it = lastWrittenHash_.constFind(old_abs);
            it != lastWrittenHash_.constEnd()) {
            lastWrittenHash_.insert(new_abs, it.value());
            lastWrittenHash_.remove(old_abs);
        }

        watcher_->addPath(new_abs); // dropped before the disk op, re-armed here
        model->renameTo(FileRef{ this, new_rel });
    }

    // What writeModel_ did. FileGone isn't a save failure: the file was deleted
    // or moved outside Suzuri — or its folder was — and the watcher hasn't
    // reported it. Callers that report failures leave it out
    enum class WriteResult_
    {
        Written,
        Failed,
        FileGone
    };

    // The single write site. Every path to disk — the debounce, the ceiling,
    // the on-exit flush, the eviction flush — funnels through here, so the
    // watcher's self-write suppression records its fingerprint in one place,
    // and a failed write shouts in one place. Leaves the model dirty on
    // failure: a buffer is never abandoned, so the next save trigger retries
    // it.
    //
    // It only ever OVERWRITES. A live buffer's file is always on disk, so a
    // missing file means it was deleted or moved outside Suzuri, its folder
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
        auto abs = absolutePathOf(model->fileRef().relative);
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
        lastWrittenHash_.insert(q_abs, qHash(bytes));
        watcher_->addPath(q_abs);

        model->setModified(false);
        DEBUG(this, "Save succeeded for {}!", abs.prettyQString());

        return WriteResult_::Written;
    }

    // A buffer's file is gone — deleted externally (reconcileChanges_) or
    // trashed in-app (moveToTrash). One step for both, since they're the same
    // event from different sources. No off-disk buffers, so the buffer follows
    // the file: drop it from the map, stop watching, forget its fingerprint,
    // announce (every view on it self-closes — this window's pop-outs, and
    // every window for a common file), free. Each view's late lastViewClosed
    // then lands on evictModel_'s stale-signal guard
    void discardModel_(AbstractFileModel* model)
    {
        auto relative = model->fileRef().relative;
        auto q_abs = absolutePathOf(relative).toQString();

        models_.remove(relative);
        watcher_->removePath(q_abs);
        lastWrittenHash_.remove(q_abs);
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
    // always reaches a LIVE Vault. On a normal window close the window (and its
    // views) tear down a full event-loop pass before
    // App::onVaultWindowDestroyed_ runs the project Vault's deleteLater; the
    // common Vault is App-owned and never dies on a window close. At app quit,
    // ~QApplication reaps top-level windows before ~QObject reaps App's Vault
    // children. So the owning Vault is alive whenever a view of its models
    // dies, and no closingDown() guard is needed
    void evictModel_(AbstractFileModel* model)
    {
        auto relative = model->fileRef().relative;

        // Stale-signal guard. External deletion (reconcileChanges_) already
        // dropped this model from the map and deleteLater'd it; its views then
        // self-close on removedFromDisk, and the LAST one firing lastViewClosed
        // lands here on a model already gone. The identity check additionally
        // refuses to free a DIFFERENT model re-inserted under the same relative
        // (delete + recreate + reopen) off an old model's late signal
        auto it = models_.constFind(relative);

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

        auto q_abs = absolutePathOf(relative).toQString();
        models_.remove(relative);
        watcher_->removePath(q_abs);
        lastWrittenHash_.remove(q_abs);
        model->deleteLater();

        DEBUG(
            this,
            "Evicted viewless model for {} (total: {})",
            relative,
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
            if (model->isModified()) {
                writeModel_(model);
            }
        }
    }

    // A setter changed the config: show it now, write it a beat later.
    // Restarting the timer on each change coalesces a slider drag into one
    // write once it settles
    void onConfigChanged_()
    {
        emit configChanged();
        configSaveTimer_->start(configSaveMs_);
    }

    // The debounced settings write, also forced by flush(). Stops the timer so
    // a forced write doesn't repeat when the timer would have fired
    void saveConfig_()
    {
        configSaveTimer_->stop();
        config_.save(root_);
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

        for (const auto& q_abs : changed) {
            auto abs = Coco::Path(q_abs);
            auto relative = relativePathOf(abs);

            auto it = models_.constFind(relative);

            if (it == models_.constEnd()) {
                continue; // not a file we hold a buffer for
            }

            auto* model = it.value();

            // External deletion: the buffer follows the file, through the same
            // discard an in-app trash uses
            if (!abs.exists()) {
                DEBUG(
                    this,
                    "External deletion: {} — closing views",
                    abs.prettyQString());
                discardModel_(model);
                continue;
            }

            // The file exists and changed. Read it — but never ACT on a failed
            // read (a file locked mid-write by the external tool): treating
            // that as "now empty" could clobber the buffer. Re-add the watch (a
            // rename may have dropped it) and defer to the next pass, when the
            // tool's final rename will have fired another event
            auto disk = Io::tryRead(abs);

            if (!disk) {
                watcher_->addPath(q_abs);
                pendingChanged_ << q_abs;
                watchReconcileTimer_->start(watchReconcileMs_);
                continue;
            }

            // An external editor's atomic-rename save can also drop our watch;
            // re-add before any branch below returns
            watcher_->addPath(q_abs);

            auto disk_hash = qHash(*disk);

            // Our own write: disk still matches the fingerprint writeModel_
            // recorded. True regardless of how far the buffer has advanced —
            // which is why we compare to the fingerprint and not to the live
            // buffer. Ignore it
            if (auto h = lastWrittenHash_.constFind(q_abs);
                h != lastWrittenHash_.constEnd() && *h == disk_hash) {
                continue;
            }

            // A genuine external change. If the tool wrote exactly what we
            // already hold, there is nothing to do — sync our fingerprint and
            // move on, so a no-op change is silent
            if (*disk == model->data()) {
                lastWrittenHash_.insert(q_abs, disk_hash);
                DEBUG(
                    this,
                    "External no-op (disk == buffer): {}",
                    abs.prettyQString());
                continue;
            }

            // An external tool re-encoded the file out of UTF-8. There's no GUI
            // here to ask, so it's adopted like any external change: the
            // undecodable sequences become U+FFFD in the buffer and reach disk
            // at the next edit. Rare, and accepted — logged so it's findable
            // when someone asks where their quotes went
            if (FileTypes::typeOf(relative) == FileTypes::Type::Text &&
                !TextFileModel::isValidUtf8(*disk)) {
                WARN(
                    this,
                    "External change isn't valid UTF-8 — adopting lossily: {}",
                    abs.prettyQString());
            }

            // Adopt the external change. reloadContent routes through the prime
            // as an UNDOABLE replace, so Ctrl+Z reverts it and the next
            // autosave rewrites the buffer — the escape hatch that lets us
            // adopt unconditionally rather than prompting. A dirty buffer's
            // unsaved edits are exactly what undo restores, so nothing is lost
            // that the user can't get back. (If a future case ever needs to
            // BLOCK an external rewrite — a merge in progress, a protected file
            // — the gate goes here, on model->isModified().)
            DEBUG(this, "External change — reload: {}", abs.prettyQString());
            model->reloadContent(*disk);
            lastWrittenHash_.insert(q_abs, disk_hash);
            model->notifyReloaded();
        }
    }
};

} // namespace Suzuri
