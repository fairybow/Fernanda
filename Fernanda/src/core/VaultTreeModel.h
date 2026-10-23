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

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>

#include <Coco/Bool.h>
#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/FileTypes.h"
#include "core/VaultTreeNode.h"

namespace Fernanda {

// The vault's file tree. One per Vault, owned by it, and shared by every tree
// view of that vault — for the Common Vault, every window's common drawer.
// Expansion and selection live in each view, not here.
//
// QtCore only (no icons), so the Vault stays GUI-free. Listing is lazy and
// synchronous: a folder is read from disk the first time a view (or indexOf)
// asks for its children — one QDir pass on the UI thread, less than the
// FileSwitcher's full-vault walk already costs.
//
// The listing admits exactly what the vault shows: no hidden entries
// (FileTypes::isHiddenName) and no unsupported files (FileTypes::isSupported)
// — the same predicates Vault::visibleFiles and BufferStore::openModel use. A
// hidden folder is never listed, so it is never watched: nothing under .git/
// ever holds a directory handle.
//
// Every listed folder is watched — the watched set IS the listed set, with no
// separate bookkeeping. On Windows each watch is an open directory handle, and
// a handle inside a folder blocks moving, renaming, or trashing it; owning the
// watcher here is what lets the Vault release them. External changes are
// coalesced and re-listed as a diff against disk, which is idempotent: an echo
// of a change the model already reflects finds nothing to do, so nothing needs
// suppressing.
//
// Each node stores only its own name; paths are built by walking parents, so
// renaming or moving a folder is one node change and its descendants follow.
// Order: folders first, then names in natural, case-insensitive order.
//
// The Vault's own disk operations update the model directly (the Vault-facing
// section below) rather than waiting on a watcher round trip, and release the
// watches inside an entry before touching it on disk.
//
// See docs/Architecture.md, "The vault tree model".
class VaultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    VaultTreeModel(const Coco::Path& root, QObject* parentVault)
        : QAbstractItemModel(parentVault)
        , root_(root)
    {
        setup_();
    }

    ~VaultTreeModel() override
    {
        TRACER;
        delete rootNode_;
    }

    using QObject::parent;

    [[nodiscard]] Coco::Path root() const noexcept { return root_; }

    // The absolute path of an entry. The invalid index is the vault root
    [[nodiscard]] Coco::Path pathOf(const QModelIndex& index) const
    {
        return absolutePathOf_(nodeOf_(index));
    }

    // Answered from the listing, not the disk — no stat. The invalid index
    // (the root) is a directory
    [[nodiscard]] bool isDir(const QModelIndex& index) const
    {
        return nodeOf_(index)->entry.isDir;
    }

    // The entry's own name, extension included — what a file's display text
    // leaves off. Answered from the listing. The invalid index (the root)
    // has an empty name
    [[nodiscard]] Coco::Path nameOf(const QModelIndex& index) const
    {
        return nodeOf_(index)->entry.name;
    }

    // The index of an entry by absolute path, listing any unlisted folder on
    // the way down. Invalid if the path is outside the vault, the root itself,
    // or not shown (missing on disk, hidden, or unsupported). Synchronous, so
    // expansion restore can expand straight through a saved chain
    [[nodiscard]] QModelIndex indexOf(const Coco::Path& absolute)
    {
        auto node = findNode_(absolute, Fetch_::Yes);
        if (!node || node == rootNode_) {
            return {};
        }

        return indexOfNode_(node);
    }

    // --- Vault-facing: around the Vault's own disk operations ---------------
    //
    // Each takes absolute paths and quietly does nothing for a path the model
    // hasn't listed — an unlisted entry holds no watch and shows in no view

    // Stop watching every listed folder at or under absolute, so no directory
    // handle of ours sits inside an entry about to be moved, renamed, or
    // trashed. A file holds none, so it's a no-op
    void releaseWatches(const Coco::Path& absolute)
    {
        if (auto node = findNode_(absolute, Fetch_::No)) {
            unwatchSubtree_(node);
        }
    }

    // Undo releaseWatches after a disk operation that failed and left the
    // entry where it was
    void restoreWatches(const Coco::Path& absolute)
    {
        if (auto node = findNode_(absolute, Fetch_::No)) {
            watchSubtree_(node);
        }
    }

    // An entry was created on disk. Re-listing its folder inserts it in sorted
    // position and applies the hide rules — one listing of one folder
    void applyAddition(const Coco::Path& absolute)
    {
        auto parent = findNode_(absolute.parent(), Fetch_::No);
        if (parent && parent->listed) {
            relist_(parent);
        }
    }

    // An entry left disk (trashed). Its node and every listed folder beneath
    // it go; releaseWatches has already dropped their watches
    void applyRemoval(const Coco::Path& absolute)
    {
        auto node = findNode_(absolute, Fetch_::No);
        if (!node || node == rootNode_) {
            return;
        }

        removeChild_(node->parent, rowOf_(node));
    }

    // An entry was renamed or moved on disk, after releaseWatches. The node
    // itself moves — same object, new name and parent — so every view keeps it
    // and its descendants expanded and selected: QTreeView holds both as
    // persistent indices, which follow a moved row. Then its listed folders
    // are watched again at their new paths.
    //
    // If the destination folder isn't listed, a partial listing can't take a
    // new row, so the node is removed instead; the destination lists fresh
    // when first expanded (and no view had it expanded, or it'd be listed). An
    // entry that wasn't listed at its old place just appears at its new one
    void applyRelocation(
        const Coco::Path& absoluteOld,
        const Coco::Path& absoluteNew)
    {
        auto node = findNode_(absoluteOld, Fetch_::No);
        auto new_parent = findNode_(absoluteNew.parent(), Fetch_::No);
        auto parent_listed = new_parent && new_parent->listed;

        if (!node || node == rootNode_) {
            if (parent_listed) {
                relist_(new_parent);
            }
            return;
        }

        VaultTreeEntry entry{ absoluteNew.name(), node->entry.isDir };

        if (!parent_listed || !isShown_(entry)) {
            removeChild_(node->parent, rowOf_(node));
            return;
        }

        moveNode_(node, new_parent, entry);
        watchSubtree_(node);
    }

    // The vault's folder is at root now, moved or spelled anew with everything
    // in it (Vault::moveRoot). Nothing is re-listed, and no node changes, so
    // every view keeps its expansion and selection. Every path the model hands
    // out is built from the root, so they all take the new one. A watch is
    // compared in the exact string it was added with, so each listed folder is
    // watched again under the new root
    void moveRoot(const Coco::Path& root)
    {
        unwatchSubtree_(rootNode_);
        root_ = root;
        watchSubtree_(rootNode_);
    }

    // --- QAbstractItemModel ------------------------------------------------

    [[nodiscard]] QModelIndex
    index(int row, int column, const QModelIndex& parent) const override
    {
        if (!hasIndex(row, column, parent)) {
            return {};
        }

        return createIndex(row, column, nodeOf_(parent)->children.at(row));
    }

    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) {
            return {};
        }

        return indexOfNode_(nodeOf_(child)->parent);
    }

    [[nodiscard]] int rowCount(const QModelIndex& parent) const override
    {
        if (parent.column() > 0) {
            return 0;
        }

        return static_cast<int>(nodeOf_(parent)->children.size());
    }

    [[nodiscard]] int columnCount(const QModelIndex&) const override
    {
        return 1;
    }

    // A file shows its stem: the extension is the badge's job
    // (VaultTreeItemDelegate). A folder shows its whole name, since
    // "Draft v1.0" has an "extension" too
    [[nodiscard]] QVariant
    data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole) {
            return {};
        }

        const auto& entry = nodeOf_(index)->entry;
        return entry.isDir ? entry.name.toQString() : entry.name.stemQString();
    }

    // Nothing is editable (rename runs through the Vault) and nothing carries
    // model drag-and-drop (the tree starts its own QDrag)
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid()) {
            return Qt::NoItemFlags;
        }

        auto item_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (!isDir(index)) {
            item_flags |= Qt::ItemNeverHasChildren;
        }

        return item_flags;
    }

    // An unlisted folder claims children so its expand arrow shows; once
    // listed, the arrow reflects what it holds (QFileSystemModel's behavior)
    [[nodiscard]] bool hasChildren(const QModelIndex& parent) const override
    {
        auto node = nodeOf_(parent);
        if (!node->entry.isDir) {
            return false;
        }

        return !node->listed || !node->children.isEmpty();
    }

    [[nodiscard]] bool canFetchMore(const QModelIndex& parent) const override
    {
        auto node = nodeOf_(parent);
        return node->entry.isDir && !node->listed;
    }

    void fetchMore(const QModelIndex& parent) override
    {
        if (canFetchMore(parent)) {
            list_(nodeOf_(parent));
        }
    }

private:
    COCO_BOOL(Fetch_)

    Coco::Path root_;
    VaultTreeNode* rootNode_ = new VaultTreeNode({ {}, true }, nullptr);

    QFileSystemWatcher* watcher_ = new QFileSystemWatcher(this);

    // Folders reported changed, re-listed together one debounce later so a git
    // checkout or a sync burst is one pass, not N — the same coalescing, and
    // the same interval, as the BufferStore's buffer reconcile
    Coco::PathList pendingDirs_{};
    Coco::Time::Debouncer* reconcileTimer_ =
        Coco::Time::newDebouncer(this, &VaultTreeModel::reconcile_);
    int reconcileMs_ = 250;

    QCollator collator_{};

    void setup_()
    {
        collator_.setNumericMode(true);
        collator_.setCaseSensitivity(Qt::CaseInsensitive);

        connect(
            watcher_,
            &QFileSystemWatcher::directoryChanged,
            this,
            &VaultTreeModel::onDirectoryChanged_);

        // The root is listed (and watched) up front. No view exists yet, so
        // the insert signals reach no one
        list_(rootNode_);
    }

    [[nodiscard]] VaultTreeNode* nodeOf_(const QModelIndex& index) const
    {
        if (!index.isValid()) {
            return rootNode_;
        }

        return static_cast<VaultTreeNode*>(index.internalPointer());
    }

    [[nodiscard]] int rowOf_(const VaultTreeNode* node) const
    {
        return static_cast<int>(node->parent->children.indexOf(node));
    }

    [[nodiscard]] QModelIndex indexOfNode_(VaultTreeNode* node) const
    {
        if (node == rootNode_) {
            return {};
        }

        return createIndex(rowOf_(node), 0, node);
    }

    [[nodiscard]] Coco::Path absolutePathOf_(const VaultTreeNode* node) const
    {
        if (node == rootNode_) {
            return root_;
        }

        return absolutePathOf_(node->parent) / node->entry.name;
    }

    [[nodiscard]] VaultTreeNode*
    childNamed_(const VaultTreeNode* dir, const Coco::Path& name) const
    {
        for (auto child : dir->children) {
            if (child->entry.name == name) {
                return child;
            }
        }

        return nullptr;
    }

    // Walk from the root to an absolute path, one component at a time. With
    // Fetch_::Yes an unlisted folder on the way is listed; with Fetch_::No the
    // walk stops there (the watcher only cares about what's already listed)
    [[nodiscard]] VaultTreeNode*
    findNode_(const Coco::Path& absolute, Fetch_ fetch)
    {
        if (!absolute.isAtOrUnder(root_)) {
            return nullptr;
        }

        auto node = rootNode_;

        // lexicallyRelative answers "." for the root itself
        for (const auto& part : absolute.lexicallyRelative(root_).toStd()) {
            if (part == ".") {
                continue;
            }

            if (!node->entry.isDir) {
                return nullptr;
            }

            if (!node->listed) {
                if (!fetch) {
                    return nullptr;
                }

                list_(node);
            }

            node = childNamed_(node, Coco::Path(part));
            if (!node) {
                return nullptr;
            }
        }

        return node;
    }

    [[nodiscard]] bool
    lessThan_(const VaultTreeEntry& a, const VaultTreeEntry& b) const
    {
        if (a.isDir != b.isDir) {
            return a.isDir;
        }

        return collator_.compare(a.name.toQString(), b.name.toQString()) < 0;
    }

    // Hidden entries and unsupported files never enter the model
    [[nodiscard]] static bool isShown_(const VaultTreeEntry& entry)
    {
        if (FileTypes::isHiddenName(entry.name.toQString())) {
            return false;
        }

        return entry.isDir || FileTypes::isSupported(entry.name);
    }

    // One folder's shown entries, sorted. QDirIterator's file info comes from
    // the directory read itself, so isDir costs no extra stat. Entries with the
    // Windows hidden attribute are skipped (no QDir::Hidden), as
    // QFileSystemModel's default filter and Coco::walkFilePaths also do
    [[nodiscard]] QList<VaultTreeEntry>
    readEntries_(const Coco::Path& dir) const
    {
        QList<VaultTreeEntry> entries{};
        QDirIterator it(
            dir.toQString(),
            QDir::AllEntries | QDir::NoDotAndDotDot);

        while (it.hasNext()) {
            auto info = it.nextFileInfo();
            VaultTreeEntry entry{ Coco::Path(info.fileName()), info.isDir() };

            if (isShown_(entry)) {
                entries << entry;
            }
        }

        std::sort(
            entries.begin(),
            entries.end(),
            [this](const VaultTreeEntry& a, const VaultTreeEntry& b) {
                return lessThan_(a, b);
            });

        return entries;
    }

    // First listing of a folder: read, insert every row, start watching
    void list_(VaultTreeNode* dir)
    {
        auto absolute = absolutePathOf_(dir);
        auto entries = readEntries_(absolute);

        if (!entries.isEmpty()) {
            beginInsertRows(
                indexOfNode_(dir),
                0,
                static_cast<int>(entries.size()) - 1);

            for (const auto& entry : entries) {
                dir->children << new VaultTreeNode(entry, dir);
            }

            endInsertRows();
        }

        dir->listed = true;

        if (!watcher_->addPath(absolute.toQString())) {
            WARN("Couldn't watch folder {}!", absolute);
        }
    }

    // Re-list a listed folder against disk: drop children that are gone (or
    // flipped between file and folder), insert the new ones in sorted
    // position. Children that didn't change keep their nodes — and with them
    // every view's expansion and selection. Hashed lookups keep this linear:
    // every autosave in the folder lands here (QSaveFile's temp-then-rename
    // is a directory change), and a daily-notes folder can hold thousands
    void relist_(VaultTreeNode* dir)
    {
        auto entries = readEntries_(absolutePathOf_(dir));

        std::unordered_map<Coco::Path, bool> is_dir_on_disk{};
        for (const auto& entry : entries) {
            is_dir_on_disk.emplace(entry.name, entry.isDir);
        }

        for (auto row = dir->children.size() - 1; row >= 0; --row) {
            const auto& entry = dir->children.at(row)->entry;
            auto it = is_dir_on_disk.find(entry.name);

            if (it == is_dir_on_disk.end() || it->second != entry.isDir) {
                removeChild_(dir, static_cast<int>(row));
            }
        }

        std::unordered_set<Coco::Path> kept_names{};
        for (auto child : dir->children) {
            kept_names.insert(child->entry.name);
        }

        for (const auto& entry : entries) {
            if (!kept_names.contains(entry.name)) {
                insertChild_(dir, entry);
            }
        }
    }

    // Where entry sorts among dir's children, not counting `except` (a node
    // about to move, which mustn't be compared against itself)
    [[nodiscard]] int sortedRow_(
        const VaultTreeNode* dir,
        const VaultTreeEntry& entry,
        const VaultTreeNode* except) const
    {
        auto row = 0;
        for (auto child : dir->children) {
            if (child != except && lessThan_(child->entry, entry)) {
                ++row;
            }
        }

        return row;
    }

    void insertChild_(VaultTreeNode* dir, const VaultTreeEntry& entry)
    {
        auto row = sortedRow_(dir, entry, nullptr);

        beginInsertRows(indexOfNode_(dir), row, row);
        dir->children.insert(row, new VaultTreeNode(entry, dir));
        endInsertRows();
    }

    // Give node a new entry (name) under a new or the same parent, at its
    // sorted place there. Qt's move signals want the destination in
    // before-the-move rows: within one parent, a node moving down lands one
    // past where it will end up, and a "move" to its own row or the next is
    // no move at all (beginMoveRows refuses it), which only needs the rename
    void moveNode_(
        VaultTreeNode* node,
        VaultTreeNode* newParent,
        const VaultTreeEntry& entry)
    {
        auto old_parent = node->parent;
        auto old_row = rowOf_(node);
        auto new_row = sortedRow_(newParent, entry, node);

        auto same_parent = old_parent == newParent;
        auto dest_row =
            (same_parent && new_row >= old_row) ? new_row + 1 : new_row;

        if (same_parent && (dest_row == old_row || dest_row == old_row + 1)) {
            node->entry = entry;
            auto index = indexOfNode_(node);
            emit dataChanged(index, index, { Qt::DisplayRole });
            return;
        }

        beginMoveRows(
            indexOfNode_(old_parent),
            old_row,
            old_row,
            indexOfNode_(newParent),
            dest_row);

        old_parent->children.removeAt(old_row);
        node->entry = entry;
        node->parent = newParent;
        newParent->children.insert(new_row, node);

        endMoveRows();

        auto index = indexOfNode_(node);
        emit dataChanged(index, index, { Qt::DisplayRole });
    }

    void removeChild_(VaultTreeNode* dir, int row)
    {
        auto child = dir->children.at(row);
        unwatchSubtree_(child);

        beginRemoveRows(indexOfNode_(dir), row, row);
        dir->children.removeAt(row);
        endRemoveRows();

        delete child;
    }

    // Every listed folder at or under node is watched again. Files are never
    // listed, so they're skipped by the same test
    void watchSubtree_(const VaultTreeNode* node)
    {
        if (!node->listed) {
            return;
        }

        auto absolute = absolutePathOf_(node);
        if (!watcher_->addPath(absolute.toQString())) {
            WARN("Couldn't watch folder {}!", absolute);
        }

        for (auto child : node->children) {
            watchSubtree_(child);
        }
    }

    // Compared in the exact string form the watch was added with
    [[nodiscard]] bool isWatched_(const VaultTreeNode* node) const
    {
        return watcher_->directories().contains(
            absolutePathOf_(node).toQString());
    }

    // A listed folder that lost its watch (see reconcile_): watch it again,
    // re-list it, and do the same for every listed folder beneath it
    void rewatchAndRelistSubtree_(VaultTreeNode* dir)
    {
        if (!isWatched_(dir)) {
            auto absolute = absolutePathOf_(dir);
            if (!watcher_->addPath(absolute.toQString())) {
                WARN("Couldn't watch folder {}!", absolute);
            }
        }

        relist_(dir);

        for (auto child : dir->children) {
            if (child->listed) {
                rewatchAndRelistSubtree_(child);
            }
        }
    }

    // Every listed folder at or under node stops being watched. Files are
    // never listed, so they're skipped by the same test
    void unwatchSubtree_(const VaultTreeNode* node)
    {
        if (!node->listed) {
            return;
        }

        watcher_->removePath(absolutePathOf_(node).toQString());

        for (auto child : node->children) {
            unwatchSubtree_(child);
        }
    }

    void onDirectoryChanged_(const QString& path)
    {
        Coco::Path dir(path);
        if (!pendingDirs_.contains(dir)) {
            pendingDirs_ << dir;
        }

        reconcileTimer_->start(reconcileMs_);
    }

    // Re-list each changed folder that is still listed and still on disk.
    // Looked up fresh per folder, since an earlier re-list in this pass may
    // have removed it. A folder that's gone is skipped here — its parent,
    // always listed and so always watched, reports the removal.
    //
    // Qt drops a watch when its folder vanishes. A folder back on disk by now
    // (deleted and recreated inside one debounce) kept its node through its
    // parent's diff, but nothing watches it or its listed subfolders, and
    // their contents may be anything: each is re-watched and re-listed
    void reconcile_()
    {
        const auto dirs = pendingDirs_;
        pendingDirs_.clear();

        for (const auto& dir : dirs) {
            auto node = findNode_(dir, Fetch_::No);
            if (!node || !node->listed || !dir.isDir()) {
                continue;
            }

            if (!isWatched_(node)) {
                rewatchAndRelistSubtree_(node);
                continue;
            }

            relist_(node);
        }
    }
};

} // namespace Fernanda
