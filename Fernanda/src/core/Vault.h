/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <type_traits>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/BufferStore.h"
#include "core/FileTypes.h"
#include "core/Io.h"
#include "core/PathKey.h"
#include "core/VaultConfig.h"
#include "core/VaultDotDir.h"
#include "core/VaultTreeModel.h"
#include "core/spell/WordList.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// The GUI-free model side of a vault: a root directory, the file tree every
// view of this vault shares, the vault's settings and dictionary, and creating,
// renaming, moving, and trashing inside it. No GUI types. Owned by App;
// borrowed by a VaultWindow.
//
// Open files' buffers aren't the Vault's. They live in the BufferStore App
// lends every Vault, one per file however many vaults contain it, and each
// file operation here tells the store what it moved or removed, so a buffer
// follows its file whichever vault's window shows it.
//
// Files are named by PathKey: absolute, so a key means the same file whichever
// vault or window holds it. Paths relative to the root are only for showing
// and saving (the switcher's rows, workspace.json).
//
// See docs/Architecture.md, "Saving" and "Watching the filesystem".
class Vault : public QObject
{
    Q_OBJECT

public:
    // root is used as given. App resolves it through symlinks first, so one
    // folder is one Vault however it was reached. bufferStore is App's, and
    // outlives every Vault
    Vault(const Coco::Path& root, BufferStore* bufferStore, QObject* parentApp)
        : QObject(parentApp)
        , rootKey_(PathKey::of(root))
        , root_(rootKey_.path())
        , bufferStore_(bufferStore)
    {
        setup_();
    }

    ~Vault() override { TRACER; }

    [[nodiscard]] Coco::Path root() const noexcept { return root_; }

    // The root as a key, for asking whether a key is this vault's
    [[nodiscard]] PathKey rootKey() const { return rootKey_; }

    // The vault's file tree, shared by every tree view of this vault in every
    // window. Views borrow it; the Vault owns it
    [[nodiscard]] VaultTreeModel* treeModel() const noexcept
    {
        return treeModel_;
    }

    // Is this the root or something inside it? Every public function that
    // takes a key answers to this, or to its stricter form (inside, and not
    // the root itself). A key is normalized, so "<root>/a/../.." is a key
    // outside the root and refused here
    [[nodiscard]] bool contains(const PathKey& key) const
    {
        return key.isAtOrUnder(rootKey_);
    }

    // Every file this vault shows — the FileSwitcher's listing.
    // Walked fresh on each call: no index and no watcher, so the list is a
    // snapshot of the moment it's asked for. Agrees with the trees by
    // construction — the same two predicates VaultTreeModel lists by. A hidden
    // folder (.fernanda/, .git/) is pruned, never walked; a file must be
    // neither hidden nor unsupported. Unsorted
    [[nodiscard]] QList<PathKey> visibleFiles() const
    {
        auto absolutes =
            Coco::walkFilePaths(root_, [](const Coco::Path& subdir) {
                return !FileTypes::isHiddenName(subdir.nameQString());
            });

        QList<PathKey> result{};

        for (const auto& absolute : absolutes) {
            if (FileTypes::isHiddenName(absolute.nameQString())) {
                continue;
            }

            if (!FileTypes::isSupported(absolute)) {
                continue;
            }

            result << PathKey::of(absolute);
        }

        return result;
    }

    // Write a pending settings change now, rather than when its timer fires.
    // Every exit path calls this beside flushing the buffers, so a change made
    // a moment before a close or a quit isn't lost. A failure isn't reported —
    // a settings file is never worth refusing a close over, and Io logged it
    void saveSettingsNow()
    {
        if (configSaveTimer_->isActive()) {
            saveConfig_();
        }
    }

    // Create a new empty file / new folder inside a directory (a vault folder,
    // or the vault root) and return its key (empty on failure). There are no
    // off-disk buffers, so the file exists on disk before any buffer opens it
    PathKey createFile(const PathKey& dir)
    {
        if (!contains(dir)) {
            WARN("Can't create a file in {}: outside the vault!", dir.path());
            return {};
        }

        auto absolute = uniqueChildPath_(dir.path(), u"Untitled"_s, u".txt"_s);

        // CreateDirs::No: a folder deleted between the right-click and the
        // click stays deleted, and the create fails
        if (!Io::write(QByteArray{}, absolute, Io::CreateDirs::No)) {
            WARN("Failed to create a new file at {}!", absolute);
            return {};
        }

        treeModel_->applyAddition(absolute);
        return PathKey::of(absolute);
    }

    PathKey createFolder(const PathKey& dir)
    {
        if (!contains(dir)) {
            WARN("Can't create a folder in {}: outside the vault!", dir.path());
            return {};
        }

        auto absolute = uniqueChildPath_(dir.path(), u"Untitled"_s, QString{});

        if (!Coco::mkdir(absolute)) {
            WARN("Failed to create a new folder at {}!", absolute);
            return {};
        }

        treeModel_->applyAddition(absolute);
        return PathKey::of(absolute);
    }

    // Create an empty file at a key named in full, making any
    // missing folders above it. This is Go to File's create: the user typed
    // the path, so the folders in it are theirs to make, where createFile's
    // folder must already be there. The vault's own folder is never recreated.
    //
    // Returns false, having made nothing, for a key that isn't inside the
    // vault, has a hidden name anywhere below the root, isn't a text file (an
    // empty PDF or image is no file at all), or already exists. Also false when
    // the disk refuses, as when a part of the path is a file; folders made
    // before the refusal stay, and the tree finds them by its watcher.
    //
    // The tree is told about the highest entry made, so a new folder appears
    // with the file inside it. The tree can't place the file itself while the
    // folder holding it has no row
    bool createFileAt(const PathKey& key)
    {
        if (!key.isUnder(rootKey_)) {
            WARN("Can't create {}: not a path inside the vault!", key.path());
            return false;
        }

        auto relative = key.relativeTo(rootKey_);

        for (auto part = relative; !part.isEmpty(); part = part.parent()) {
            if (FileTypes::isHiddenName(part.nameQString())) {
                WARN(
                    "Can't create {}: a name begins with a period!",
                    key.path());
                return false;
            }
        }

        auto absolute = key.path();

        if (FileTypes::typeOf(absolute) != FileTypes::Type::Text) {
            WARN("Can't create {}: not a text file!", absolute);
            return false;
        }

        if (!root_.isDir()) {
            WARN("Can't create {}: the vault folder is gone!", absolute);
            return false;
        }

        if (absolute.exists()) {
            WARN("Can't create {}: it already exists!", absolute);
            return false;
        }

        auto highest = absolute;

        for (auto folder = absolute.parent();
             PathKey::of(folder).isUnder(rootKey_) && !folder.exists();
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

    // Rename a file or folder in place — same parent, new leaf name. entry is
    // an existing entry in this vault; newLeaf is its new on-disk name (a
    // file's extension is already reattached by the caller — the tree hides
    // it). Returns the new key (empty on failure). A rename is a relocation
    // whose parent doesn't change, so it funnels through relocate_ (below)
    // exactly as a move does — the disk op, watcher suppression, and buffer
    // re-keying are shared. Declines cleanly on a locked entry, never corrupts.
    //
    // newLeaf must be one name. Anything with a separator, a "." or "..", or a
    // root would make this a move, and is refused
    PathKey rename(const PathKey& entry, const QString& newLeaf)
    {
        auto leaf = Coco::Path(newLeaf);

        if (leaf.isEmpty() || !leaf.isPlain() || leaf.name() != leaf) {
            WARN(
                "Can't rename {} to {}: not a single name!",
                entry.path(),
                leaf);
            return {};
        }

        return relocate_(entry, PathKey::of(entry.path().parent() / newLeaf));
    }

    // Move a file or folder into a different directory. A move is a rename with
    // a new parent, so it shares relocate_'s core: the leaf name is preserved,
    // so the new path is destDir/name. entry is an existing entry in this
    // vault; destDir is where it lands (a tree folder, or the vault root for a
    // drop on empty space). Any open buffer at or under the entry is re-keyed
    // just as a folder rename does. Returns the new key (empty on failure — a
    // name collision, a locked entry, or an invalid target). Declines cleanly,
    // never corrupts.
    //
    // The guards below are defensive, behind the tree's advisory ones: the tree
    // suppresses these drops (no indicator, ignored), but the disk op stays
    // authoritative, so a race or a future non-tree caller can't drive a
    // corrupting move through. Vault scope is relocate_'s to check, for rename
    // and move alike. The no-op case (dropping an entry into its own current
    // parent) is the tree's to filter — it's a pure path comparison needing no
    // Vault, and letting it reach here would only turn a harmless nothing into
    // a spurious failure warning
    PathKey move(const PathKey& entry, const PathKey& destDir)
    {
        if (!destDir.path().isDir()) {
            WARN("Move target {} is not a directory!", destDir.path());
            return {};
        }

        // Nothing can move into itself or its own descendant. isAtOrUnder
        // compares whole names (no false prefix hit) and covers the
        // entry-onto-itself case in the same call. Only folders can fail this:
        // the destination is a directory, and a directory is never at or under
        // a file
        if (destDir.isAtOrUnder(entry)) {
            WARN("Can't move {} into itself or a descendant!", entry.path());
            return {};
        }

        return relocate_(
            entry,
            PathKey::of(destDir.path() / entry.path().name()));
    }

    // Move a file or folder to the system trash. entry is an existing entry in
    // this vault — never the root. Every open buffer at or under it is
    // flushed first, so the trashed copy holds the latest text, then discarded
    // once the entry is gone: the same step the watcher's deletion branch
    // takes, so every view on those buffers, in every window, closes.
    //
    // Returns false with nothing changed on disk or in any buffer. One bool,
    // but each refusal logs its own reason, so the log says which it was: out
    // of scope, the root, already gone, a failed flush (the store names the
    // file), or the system trash refusing. There is no local-trash fallback
    bool moveToTrash(const PathKey& entry)
    {
        // Same authority relocate_ holds: an entry outside this vault isn't
        // this vault's to remove
        if (!contains(entry)) {
            WARN("Trash refused — {} is outside the vault!", entry.path());
            return false;
        }

        // The tree never offers the root, but this is the one place a future
        // caller has to be stopped from trashing the whole vault. Keys are
        // normalized, so every spelling of the root compares equal here
        if (entry == rootKey_) {
            WARN("Trash refused — {} is the vault root!", entry.path());
            return false;
        }

        auto absolute = entry.path();

        // Named here rather than left to read as the system trash refusing.
        // If it held open buffers, the watcher's deletion branch handles them
        if (!absolute.exists()) {
            WARN("Trash refused — {} no longer exists!", absolute);
            return false;
        }

        // Flush BEFORE unwatching (BufferStore::flushAtOrUnder says why). Any
        // failure refuses the whole delete before disk is touched — trashing a
        // file whose buffer never reached it would lose that text. Every
        // buffer under the entry is the store's, so a file open only in
        // another vault's window, where the vaults nest, is saved too
        if (!bufferStore_->flushAtOrUnder(entry)) {
            WARN("Trash refused — {} couldn't be saved first!", absolute);
            return false;
        }

        // Unwatch before the disk op, as relocate_ does: every watch inside
        // the entry, buffers' and trees' (BufferStore::unwatchAtOrUnder says
        // why). A flush's own watcher echo may still be queued; by the time
        // it's reconciled the buffer is gone from the store, so it's skipped
        bufferStore_->unwatchAtOrUnder(entry);

        if (!Coco::moveToTrash(absolute)) {
            bufferStore_->rewatchAtOrUnder(entry);

            WARN(
                "Trash refused — the system trash wouldn't take {}!",
                absolute);
            return false;
        }

        // No event-loop turn between the disk op and here, so no autosave tick
        // can write a buffer back into the gap and resurrect the entry
        auto discarded = bufferStore_->applyRemoval(entry);

        DEBUG(
            this,
            "Moved {} to the system trash ({} open buffer(s) discarded)",
            absolute,
            discarded);

        return true;
    }

    // The vault's folder was deleted or moved, outside Fernanda or from inside
    // another vault: make it again, empty, in place, spelled as root spells it.
    // Only App calls this, and only for the Common Vault — a standing folder,
    // rebuilt at startup anyway — while a project vault whose folder vanishes
    // has its window closed instead. The same Vault object stays, since every
    // project window holds a pointer to it. Every open buffer under the root is
    // also rechecked: their files went with the folder, and a clean buffer the
    // watcher missed would otherwise sit open until typed into.
    //
    // root names the same place, and may spell it differently: where letter
    // case is ignored, the folder that left may have been respelled
    // (respellRoot), and the new one is made in the spelling App gives. A
    // changed spelling is announced (rootRespelled). Refused for a key that
    // names another folder: this makes the vault's folder again, and doesn't
    // move it.
    //
    // The settings and words are taken again from the new folder, which has
    // none, so they return to their defaults and every window applies them. A
    // pending settings save is dropped first, so the old settings aren't
    // written into the new folder. The ignored words go too: they were ignored
    // in the folder that's gone. Returns false if the folder can't be made
    bool recreateRoot(const PathKey& root)
    {
        if (root.isEmpty() || !(root == rootKey_)) {
            WARN(
                "Can't recreate vault {} as {}: not the same folder!",
                root_,
                root.path());
            return false;
        }

        if (!root.path().isDir() && !Coco::mkpath(root.path())) {
            CRITICAL(this, "Couldn't recreate vault folder {}!", root.path());
            return false;
        }

        auto respelled = root.path() != root_;
        rootKey_ = root;
        root_ = rootKey_.path();

        ensureVaultDotDir(root_);
        treeModel_->relistRoot(root_);
        bufferStore_->recheckAtOrUnder(rootKey_);

        configSaveTimer_->stop();
        config_ = VaultConfig{};
        config_.load(root_);
        dictionary_ = WordList::read(dictionaryPath_()).value_or(WordList{});
        ignoredWords_ = WordList{};

        emit configChanged();
        emit wordsChanged();

        if (respelled) {
            emit rootRespelled();
        }

        INFO(this, "Recreated vault folder {}", root_);
        return true;
    }

    // The vault's folder is where it was, spelled as root spells it: a folder
    // at or above the root was renamed by letter case alone, where case is
    // ignored. Only App calls this, when another vault's rename leaves this
    // vault's folder in place. root, the tree, and every path they hand out
    // take the new spelling, and rootRespelled is announced so the windows'
    // own copies follow.
    //
    // Refused for a key that names another folder: a vault doesn't follow its
    // folder to a new place (see docs/Architecture.md, "Identity"). A spelling
    // that is already the vault's changes nothing and announces nothing
    void respellRoot(const PathKey& root)
    {
        if (root.isEmpty() || !(root == rootKey_)) {
            WARN(
                "Can't respell vault {} as {}: not the same folder!",
                root_,
                root.path());
            return;
        }

        if (root.path() == root_) {
            return;
        }

        rootKey_ = root;
        root_ = rootKey_.path();
        treeModel_->respellRoot(root_);

        INFO(this, "Vault folder now spelled {}", root_);
        emit rootRespelled();
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
    // vault, kept in .fernanda/dictionary.txt and committed with the vault.
    // Read at construction; a change made to the file by hand shows the next
    // time the vault opens, or at the next addToDictionary
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

    // root() and rootKey() are spelled anew (respellRoot, or recreateRoot
    // making the folder in another spelling), for anything that keeps its own
    // copy of the root's spelling
    void rootRespelled();

private:
    PathKey rootKey_;
    Coco::Path root_; // rootKey_'s path, declared after it to be made from it
    BufferStore* bufferStore_;

    // Per-vault settings, read at construction and saved a beat after the last
    // change: a dragged size slider changes it many times a second, and one
    // write per settle is enough. A pending save is also forced by
    // saveSettingsNow, which every exit path calls
    VaultConfig config_{};
    Coco::Time::Debouncer* configSaveTimer_ =
        Coco::Time::newDebouncer(this, &Vault::saveConfig_);
    int configSaveMs_ = 500;

    static inline const QString DICTIONARY_FILE_NAME_ = u"dictionary.txt"_s;

    WordList dictionary_{};
    WordList ignoredWords_{};

    // Lists and watches the folders the tree views have opened. The store's
    // watcher tracks the files it holds buffers for; this one the folders the
    // trees show
    VaultTreeModel* treeModel_ = new VaultTreeModel(root_, this);

    void setup_()
    {
        ensureVaultDotDir(root_);
        config_.load(root_);
        dictionary_ = WordList::read(dictionaryPath_()).value_or(WordList{});

        // So the store can let go of this tree's folder watches around any
        // vault's disk operation, where the vaults nest
        bufferStore_->addTree(treeModel_);
    }

    [[nodiscard]] Coco::Path dictionaryPath_() const
    {
        return vaultDotDir(root_) / DICTIONARY_FILE_NAME_;
    }

    // "Untitled.txt", then "Untitled 1.txt", ... — the first not already
    // present inside absoluteDir. An empty ext gives a folder name. Absolute
    // in, absolute out; the caller makes the key. Deliberately not translated:
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

    // The shared core of rename (in-place) and move (new parent): both relocate
    // an entry from oldKey to newKey, and everything past the target path is
    // identical. The store's watches on every buffer AT or UNDER the old path
    // are dropped BEFORE the disk op, or the disappearance of `old` reports as
    // an external deletion and closes its views. They're restored on a failed
    // disk op, where nothing else changes, and re-armed under the new path by
    // the re-key. Returns the new key (empty on failure)
    PathKey relocate_(const PathKey& oldKey, const PathKey& newKey)
    {
        // Both ends must be inside this vault, or a buffer would be keyed, and
        // workspace.json would save a path, outside it. The tree already
        // refuses these, but this is the authoritative refusal the callers are
        // entitled to assume
        if (!contains(oldKey) || !contains(newKey)) {
            WARN(
                "Relocation of {} to {} leaves the vault!",
                oldKey.path(),
                newKey.path());
            return {};
        }

        auto old_path = oldKey.path();
        auto new_path = newKey.path();

        // Every watch inside the entry goes first, the buffers' and every
        // vault tree's, this one's included (BufferStore::unwatchAtOrUnder
        // says why)
        bufferStore_->unwatchAtOrUnder(oldKey);

        if (!renameOnDisk_(old_path, new_path)) {
            bufferStore_->rewatchAtOrUnder(oldKey);

            WARN("Failed to relocate {} to {}!", old_path, new_path);
            return {};
        }

        // Every vault's tree, this one's included, then the buffers
        bufferStore_->applyRelocation(oldKey, newKey);

        DEBUG(this, "Relocated {} -> {}", old_path, new_path);
        return newKey;
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
        auto base = sibling.nameQString() + u".fernanda-rename"_s;

        auto candidate = dir / base;
        auto n = 0;

        while (candidate.exists()) {
            candidate = dir / (base + QString::number(++n));
        }

        return candidate;
    }

    // A setter changed the config: show it now, write it a beat later.
    // Restarting the timer on each change coalesces a slider drag into one
    // write once it settles
    void onConfigChanged_()
    {
        emit configChanged();
        configSaveTimer_->start(configSaveMs_);
    }

    // The debounced settings write, also forced by saveSettingsNow. Stops the
    // timer so a forced write doesn't repeat when the timer would have fired
    void saveConfig_()
    {
        configSaveTimer_->stop();
        config_.save(root_);
    }
};

} // namespace Fernanda
