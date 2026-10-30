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

#include <QAction>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonValue>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QPoint>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/VaultTreeModel.h"
#include "ui/OpenMode.h"
#include "ui/RenameDialog.h"
#include "ui/sidebar/VaultTreeView.h"

namespace Fernanda::Ui {

// A tree view over its vault's VaultTreeModel. The model is the Vault's and is
// shared by every tree of that vault — every window's common drawer sits on the
// Common Vault's one model — so this pane owns only its view and that view's
// state (expansion, selection). It never calls the Vault: it emits absolute
// paths (an activated file, a directory a new entry should land in, an entry to
// rename, move, or delete), and the Sidebar maps them to the right Vault
class VaultFileTree : public QWidget
{
    Q_OBJECT

public:
    VaultFileTree(VaultTreeModel* treeModel, QWidget* parentSplitter)
        : QWidget(parentSplitter)
        , model_(treeModel)
    {
        setup_();
    }

    ~VaultFileTree() override { TRACER; }

    // Hover the row under the cursor, or none, after the tree has moved under
    // a still cursor (see VaultTreeView::syncHover)
    void syncHover() { tree_->syncHover(); }

    // Persist which folders are expanded, as an opaque blob for WorkspaceFile
    // to place in workspace.json. VaultFileTree owns the SHAPE of its own
    // state; WorkspaceFile owns WHERE it lives and under which key — the same
    // split TabPaneTree's tree blob already follows, so the two persistable
    // panes share a contract (own-state JSON in / out) without sharing a
    // signature. Records only folders reachable through an all-expanded chain
    // from the root (see collectExpanded_), as vault-relative pretty paths —
    // the same path form the tab entries use
    [[nodiscard]] QJsonArray serializeExpansion() const
    {
        QJsonArray expanded{};
        collectExpanded_(QModelIndex{}, expanded);
        return expanded;
    }

    // Re-expand the saved folders. Synchronous: indexOf lists any unlisted
    // folder on the way to a path, so each saved folder is reachable the moment
    // it's asked for. collectExpanded_ writes every parent before its children,
    // so walking the array in order expands each chain from the top down. A
    // folder gone since save resolves to an invalid index and is skipped. Each
    // expand is bracketed by applyingRestore_ so it isn't mistaken for a user
    // toggle
    void restoreExpansion(const QJsonArray& expanded)
    {
        for (const auto& value : expanded) {
            auto relative = value.toString();
            if (relative.isEmpty()) {
                continue;
            }

            auto index = model_->indexOf(model_->root() / Coco::Path(relative));
            if (!index.isValid() || !model_->isDir(index)) {
                continue;
            }

            ++applyingRestore_;
            tree_->expand(index);
            --applyingRestore_;
        }
    }

    // Show an entry this tree has just made: open every folder above it,
    // select it, and scroll it into view. The folders opened count as the
    // user's own, so they're saved with the workspace (expansionChanged). An
    // entry the model can't find, as when its folder went in the meantime, is
    // left alone
    void reveal(const Coco::Path& absolute)
    {
        auto index = model_->indexOf(absolute);
        if (!index.isValid()) {
            return;
        }

        for (auto folder = index.parent(); folder.isValid();
             folder = folder.parent()) {
            tree_->expand(folder);
        }

        tree_->selectionModel()->setCurrentIndex(
            index,
            QItemSelectionModel::ClearAndSelect);
        tree_->scrollTo(index);
    }

signals:
    // A file (never a directory) was double-clicked. Its OpenMode is resolved
    // downstream (ReplaceActive unless the active tab is pinned), so this
    // carries no mode — unlike the menu opens below
    void fileActivated(const Coco::Path& absolute);

    // A file context-menu open. Unlike a double-click, the item chosen IS the
    // placement, so the mode rides along: "Open in new tab" (NewTab), "Open to
    // the right" (SplitRight), "Open in new window" (NewWindow) — none of which
    // ever replace the active tab. The Sidebar tags it with this vault and
    // forwards it to the window's one open path
    void openRequested(const Coco::Path& absolute, OpenMode mode);

    // The context menu asked to create a new file / folder inside this absolute
    // directory (a folder in the tree, or the vault root for empty space). The
    // Sidebar routes it to the owning Vault, which does the disk work
    void newFileRequested(const Coco::Path& absoluteDir);
    void newFolderRequested(const Coco::Path& absoluteDir);

    // The context menu asked to rename an entry in place: absoluteOld is the
    // existing file/folder, newLeaf its new on-disk name (a file's extension is
    // already reattached — the dialog edits the stem). The Sidebar routes it to
    // the owning Vault, which renames and re-keys any open buffer
    void renameRequested(const Coco::Path& absoluteOld, const QString& newLeaf);

    // The context menu asked to delete an entry, and the user confirmed. The
    // Sidebar routes it to the owning Vault, which flushes and trashes it and
    // discards any open buffer at or under it — every view on those buffers
    // closes, in every window for a common file
    void deleteRequested(const Coco::Path& absolute);

    // A committed move-drop in this tree: move absoluteSource into
    // absoluteDestDir (a folder here, or the vault root). Relayed straight from
    // the view; the Sidebar maps it to this tree's owning Vault, which does the
    // disk move and re-keys any open buffer under the moved entry
    void moveRequested(
        const Coco::Path& absoluteSource,
        const Coco::Path& absoluteDestDir);

    // The user expanded or collapsed a folder. Programmatic restore expansions
    // are suppressed (applyingRestore_), so this fires only for a genuine user
    // toggle — WorkspaceFile hangs a debounced workspace.json save on it
    // without a restore churning writes
    void expansionChanged();

private:
    // The Vault's, borrowed. Declared before tree_, which is built from it
    VaultTreeModel* model_;

    // Owns entry drag-to-move. A VaultTreeView, not a plain QTreeView, so the
    // pane can swap the view type without touching anything outside it. Sets
    // the model on itself at construction
    VaultTreeView* tree_ = new VaultTreeView(model_, this);

    // Guard around each programmatic expand() in restoreExpansion.
    // QTreeView::expand emits `expanded` synchronously, so a count > 0 at
    // emission time marks that emission as ours and suppresses the
    // expansionChanged it would otherwise raise — only genuine user toggles
    // schedule a save
    int applyingRestore_ = 0;

    void setup_()
    {
        // One column (the name), so the header has nothing to say. The model
        // carries no ItemIsEditable, so no edit trigger can start an inline
        // rename behind the Vault
        tree_->setHeaderHidden(true);

        // Double-click opens a file; a directory just expands (default Qt)
        connect(
            tree_,
            &QTreeView::doubleClicked,
            this,
            &VaultFileTree::onActivated_);

        // Right-click menu. Handled on the tree, which alone knows the index
        // under the cursor — the Sidebar answering "which vault?" would
        // otherwise reach through this widget's internals. The view sits on the
        // viewport, so pos and indexAt / mapToGlobal are all viewport
        // coordinates
        tree_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(
            tree_,
            &QTreeView::customContextMenuRequested,
            this,
            &VaultFileTree::onContextMenu_);

        // A committed move-drop. The view has already scoped and validated it
        // (same vault, real move); we just relay the absolute paths up for the
        // Sidebar to route to this tree's Vault
        connect(
            tree_,
            &VaultTreeView::moveRequested,
            this,
            &VaultFileTree::moveRequested);

        // Expansion persistence. A user expand/collapse nudges a debounced
        // workspace save; the applyingRestore_ guard keeps restore's own
        // expands from counting as user action
        connect(
            tree_,
            &QTreeView::expanded,
            this,
            &VaultFileTree::onExpandedOrCollapsed_);

        connect(
            tree_,
            &QTreeView::collapsed,
            this,
            &VaultFileTree::onExpandedOrCollapsed_);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(tree_);
    }

    void onActivated_(const QModelIndex& index)
    {
        if (!index.isValid() || model_->isDir(index)) {
            return;
        }

        emit fileActivated(model_->pathOf(index));
    }

    void onContextMenu_(const QPoint& pos)
    {
        auto global = tree_->viewport()->mapToGlobal(pos);
        auto index = tree_->indexAt(pos);

        // Empty space: create in the vault root. The root has no in-tree rename
        // — renaming the vault folder itself is ManageVaults' job
        if (!index.isValid()) {
            QMenu menu(this);
            addCreateActions_(menu, model_->root());
            menu.exec(global);
            return;
        }

        auto absolute = model_->pathOf(index);
        auto is_dir = model_->isDir(index);

        QMenu menu(this);

        // A folder can hold new entries (you can't nest under a file, so a file
        // gets none); a file instead gets the three explicit opens — Open in
        // new tab / Open to the right / Open in new window — since a
        // double-click replaces the active tab unless it's pinned, and these
        // are how you force a new tab, a split, or a window. Both kinds then
        // offer Rename, and Delete behind its own separator so the destructive
        // item sits apart
        if (is_dir) {
            addCreateActions_(menu, absolute);
            menu.addSeparator();
        } else {
            addOpenActions_(menu, absolute);
            menu.addSeparator();
        }
        addRenameAction_(menu, absolute, is_dir);
        menu.addSeparator();
        addDeleteAction_(menu, absolute, is_dir);

        menu.exec(global);
    }

    // Transient actions wired straight to intents — VaultFileTree owns no
    // QActions, so nothing here collides with the menu bar's (root-only,
    // selection-blind) create actions
    void addCreateActions_(QMenu& menu, const Coco::Path& targetDir)
    {
        connect(
            menu.addAction(tr("New file")),
            &QAction::triggered,
            this,
            [this, targetDir] { emit newFileRequested(targetDir); });

        connect(
            menu.addAction(tr("New folder")),
            &QAction::triggered,
            this,
            [this, targetDir] { emit newFolderRequested(targetDir); });
    }

    // Transient, like the create actions — VaultFileTree owns no QAction, so
    // nothing here collides with a menu-bar open action. Each item emits
    // openRequested with the placement it names; the Sidebar tags the vault and
    // the window opens it. None of the three replace the active tab: NewTab
    // adds a tab, SplitRight opens in a fresh right-hand split, NewWindow opens
    // in a fresh pop-out
    void addOpenActions_(QMenu& menu, const Coco::Path& absolute)
    {
        connect(
            menu.addAction(tr("Open in new tab")),
            &QAction::triggered,
            this,
            [this, absolute] {
                emit openRequested(absolute, OpenMode::NewTab);
            });

        connect(
            menu.addAction(tr("Open to the right")),
            &QAction::triggered,
            this,
            [this, absolute] {
                emit openRequested(absolute, OpenMode::SplitRight);
            });

        connect(
            menu.addAction(tr("Open in new window")),
            &QAction::triggered,
            this,
            [this, absolute] {
                emit openRequested(absolute, OpenMode::NewWindow);
            });
    }

    void addRenameAction_(QMenu& menu, const Coco::Path& absolute, bool isDir)
    {
        connect(
            menu.addAction(tr("Rename…")),
            &QAction::triggered,
            this,
            [this, absolute, isDir] { promptRename_(absolute, isDir); });
    }

    // Show the rename prompt for one entry, then emit the resolved intent. A
    // file edits its stem with the extension preserved; a folder edits its
    // whole name (a folder's "extension" is not meaningful)
    void promptRename_(const Coco::Path& absolute, bool isDir)
    {
        auto name = absolute.nameQString();

        QString initial_stem{};
        QString suffix{};
        if (isDir) {
            initial_stem = name;
        } else {
            initial_stem = absolute.stemQString();
            suffix = name.mid(initial_stem.size());
        }

        RenameDialog dialog(
            this,
            tr("Rename"),
            absolute.parent(),
            name,
            initial_stem,
            suffix);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        emit renameRequested(absolute, dialog.newLeaf());
    }

    void addDeleteAction_(QMenu& menu, const Coco::Path& absolute, bool isDir)
    {
        connect(
            menu.addAction(tr("Delete")),
            &QAction::triggered,
            this,
            [this, absolute, isDir] { promptDelete_(absolute, isDir); });
    }

    // Confirm, then emit the resolved intent. Always asked — there is no "Don't
    // ask again" option. Cancel is the default and the escape button, so Enter
    // or Esc on a misclick changes nothing. A folder's prompt says its contents
    // go too. A plain QMessageBox: unlike RenameDialog there's nothing to
    // validate
    void promptDelete_(const Coco::Path& absolute, bool isDir)
    {
        auto name = absolute.nameQString();

        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);

        if (isDir) {
            box.setWindowTitle(tr("Delete folder"));
            box.setText(tr("Are you sure you want to delete the folder \"%1\"?")
                            .arg(name));
            box.setInformativeText(
                tr("The folder and everything inside it will be moved to the "
                   "system trash."));
        } else {
            box.setWindowTitle(tr("Delete file"));
            box.setText(
                tr("Are you sure you want to delete \"%1\"?").arg(name));
            box.setInformativeText(tr("It will be moved to the system trash."));
        }

        auto* delete_button =
            box.addButton(tr("Delete"), QMessageBox::DestructiveRole);
        auto* cancel_button = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(cancel_button);
        box.setEscapeButton(cancel_button);

        box.exec();
        if (box.clickedButton() != delete_button) {
            return;
        }

        emit deleteRequested(absolute);
    }

    // --- Folder expansion ---------------------------------------

    void onExpandedOrCollapsed_(const QModelIndex&)
    {
        // A programmatic expand from restoreExpansion is bracketed by
        // applyingRestore_; only a genuine user toggle gets through to schedule
        // a save
        if (applyingRestore_ == 0) {
            emit expansionChanged();
        }
    }

    // Record every folder reachable through an all-expanded chain from parent —
    // recurse only into expanded folders, so a folder is stored only if it AND
    // its ancestors are expanded. Qt keeps a collapsed folder's expanded
    // descendants flagged, but that state can't be restored without first
    // re-expanding the ancestor, so storing only the visible chain keeps the
    // saved set one restore reproduces exactly (every entry's parents are
    // stored too). Reads isExpanded as ground truth — no parallel bookkeeping
    // to drift
    void collectExpanded_(const QModelIndex& parent, QJsonArray& out) const
    {
        auto rows = model_->rowCount(parent);
        for (auto row = 0; row < rows; ++row) {
            auto child = model_->index(row, 0, parent);
            if (!model_->isDir(child) || !tree_->isExpanded(child)) {
                continue;
            }

            out.append(model_->pathOf(child)
                           .lexicallyRelative(model_->root())
                           .prettyQString());

            collectExpanded_(child, out); // only expanded folders recurse
        }
    }
};

} // namespace Fernanda::Ui
