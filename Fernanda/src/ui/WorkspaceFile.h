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

#include <functional>
#include <utility>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QSplitter>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/CoreConstants.h"
#include "core/Io.h"
#include "core/JsonIo.h"
#include "core/VaultDotDir.h"
#include "core/WorkspaceKeys.h"
#include "ui/BaseWindow.h"
#include "ui/sidebar/Drawer.h"
#include "ui/sidebar/VaultFileTree.h"
#include "ui/tabs/TabPaneTree.h"

namespace Fernanda::Ui {

// Machine-local workspace persistence for one VaultWindow AND its pop-outs. A
// small QObject owning a debounce timer and a set of connections to the windows
// it watches. It persists workspace; it coordinates nothing.
//
// Writes <vaultRoot>/.fernanda/workspace.json, NOT committed: a
// .fernanda/.gitignore naming it is written when the dot-folder is first
// created (ensureVaultDotDir). Workspace refs are vault-relative PATHS, with no
// id.
//
// The path<->view translation, the recent files, and the pop-out lifecycle
// all belong to VaultWindow (the composition root), so they arrive as Hooks:
// this class knows geometry, sidebar sizes, and the opaque tree blob — never
// what a page IS or how a window is made. The recent files are read when a save
// runs, like a view's cursor, so a new one schedules no save of its own.
//
// Restore runs once, from VaultWindow::setup_, BEFORE the window is shown, so
// geometry lands pre-show. The MAIN window's change sources are wired only
// AFTER restore (startObserving_); pop-outs are wired as they're created
// (observePopout), and their restore-time emissions simply coalesce into the
// one benign post-launch write the geometry save already produces.
class WorkspaceFile : public QObject
{
    Q_OBJECT

public:
    // A pop-out to persist, or one just handed back for restore: the window
    // (for geometry) and its tree (for the tab layout). BaseWindow*, not
    // PopoutWindow*, so this class stays decoupled from the pop-out type —
    // everything it touches is QWidget / BaseWindow API
    struct PopoutHandle
    {
        BaseWindow* window = nullptr;
        TabPaneTree* tree = nullptr;
    };

    // Everything VaultWindow provides that this class can't know itself: the
    // page<->JSON translation (path resolution lives in VaultWindow) and the
    // pop-out lifecycle (VaultWindow is the single owner of creation). Bundled
    // so the constructor stays readable
    struct Hooks
    {
        // A page -> tab entry, or a null value to omit it (an unknown page
        // type)
        std::function<QJsonValue(QWidget*)> describePage{};

        // A tab entry -> a live page (+ title), or a null page to drop it (a
        // file that no longer exists)
        std::function<TabPaneTree::RestoredPage(const QJsonValue&)> buildPage{};

        // Every live pop-out, for serialization
        std::function<QList<PopoutHandle>()> enumeratePopouts{};

        // Create one wired-but-hidden pop-out for restore. VaultWindow tracks
        // it and wires its requests; this class fills it and decides to show or
        // discard it
        std::function<PopoutHandle()> createPopout{};

        // The window's recent files, to save and to restore. Restored after
        // the tabs, so the saved order replaces the one their restore
        // recorded
        std::function<QJsonValue()> describeRecentFiles{};
        std::function<void(const QJsonValue&)> restoreRecentFiles{};
    };

    WorkspaceFile(
        BaseWindow* window,
        TabPaneTree* tree,
        QSplitter* sidebarSplitter,
        VaultFileTree* fileTree,
        VaultFileTree* commonTree,
        Drawer* commonDrawer,
        const Coco::Path& vaultRoot,
        Hooks hooks,
        QObject* parent)
        : QObject(parent)
        , window_(window)
        , tree_(tree)
        , sidebarSplitter_(sidebarSplitter)
        , fileTree_(fileTree)
        , commonTree_(commonTree)
        , commonDrawer_(commonDrawer)
        , vaultRoot_(vaultRoot)
        , hooks_(std::move(hooks))
    {
    }

    ~WorkspaceFile() override { TRACER; }

    // Read workspace.json and apply it — main window, then pop-outs — THEN
    // begin observing the main window. A missing file (first run) restores
    // nothing and just starts observing, so the window persists wherever it
    // first opens
    void restore()
    {
        auto root = JsonIo::read(workspacePath_());

        if (!root.isEmpty()) {
            auto main = root.value(WorkspaceKeys::MAIN).toObject();

            if (auto geo = main.value(WorkspaceKeys::GEOMETRY).toString();
                !geo.isEmpty()) {
                window_->restoreGeometry(
                    QByteArray::fromBase64(geo.toLatin1()));
            }

            if (auto sidebar =
                    main.value(WorkspaceKeys::SIDEBAR_SIZES).toString();
                !sidebar.isEmpty()) {
                sidebarSplitter_->restoreState(
                    QByteArray::fromBase64(sidebar.toLatin1()));
            }

            restoreExpansion_(main.value(WorkspaceKeys::EXPANDED).toObject());

            // The drawer applies its flag and height now and sizes itself on
            // first show. An absent or unusable object leaves it collapsed, as
            // constructed
            if (commonDrawer_) {
                commonDrawer_->restoreState(
                    main.value(WorkspaceKeys::COMMON_DRAWER).toObject());
            }

            if (auto tree_node = main.value(WorkspaceKeys::TREE).toObject();
                !tree_node.isEmpty()) {
                tree_->restore(tree_node, hooks_.buildPage);
            }

            const auto popouts = root.value(WorkspaceKeys::POPOUTS).toArray();
            for (const auto& entry : popouts) {
                restorePopout_(entry.toObject());
            }

            hooks_.restoreRecentFiles(main.value(WorkspaceKeys::RECENT_FILES));
        }

        startObserving_();
    }

    // A guaranteed synchronous write — the workspace twin of the buffer flush
    // at close. Flushes any pending debounce, then writes
    void saveNow()
    {
        debounce_->stop();
        writeToDisk_();
    }

    // Observe a pop-out for saves: its geometry, its tree's structure, and its
    // destruction (closing one changes the persisted workspace). VaultWindow
    // calls this whenever it makes a pop-out — the drag-out path and restore
    // both funnel through its makeWiredPopout_
    void observePopout(BaseWindow* window, TabPaneTree* tree)
    {
        connect(
            window,
            &BaseWindow::geometryChanged,
            this,
            &WorkspaceFile::scheduleSave_);

        connect(
            tree,
            &TabPaneTree::layoutChanged,
            this,
            &WorkspaceFile::scheduleSave_);

        connect(
            window,
            &QObject::destroyed,
            this,
            &WorkspaceFile::scheduleSave_);
    }

private:
    static constexpr int SCHEMA_VERSION_ = 1;

    BaseWindow* window_ = nullptr;
    TabPaneTree* tree_ = nullptr;
    QSplitter* sidebarSplitter_ = nullptr;

    // The two sidebar trees whose folder expansion rides in workspace.json.
    // commonTree_ is null when this window is the common vault itself (no
    // common drawer). Opaque to this class — it calls their serialize/restore
    // and observes expansionChanged, nothing more
    VaultFileTree* fileTree_ = nullptr;
    VaultFileTree* commonTree_ = nullptr;

    // The common drawer's { collapsed, height }. Null alongside commonTree_.
    // Opaque like the trees — serializeState / restoreState and stateChanged,
    // nothing more
    Drawer* commonDrawer_ = nullptr;

    Coco::Path vaultRoot_;
    Hooks hooks_;

    // Workspace churns constantly (drags, resizes, splits), so coalesce writes.
    // Separate from the vault's autosave pair — different cadence, different
    // file. Single-shot (Coco::Time::Debouncer), so it fires once per burst and
    // saveNow's stop() pre-empts a pending one
    Coco::Time::Debouncer* debounce_ =
        Coco::Time::newDebouncer(this, &WorkspaceFile::writeToDisk_);
    int debounceMs_ = 500;

    [[nodiscard]] Coco::Path workspacePath_() const
    {
        return vaultDotDir(vaultRoot_) / WORKSPACE_FILE_NAME;
    }

    // The MAIN window's change sources, wired AFTER restore so rebuilding its
    // tree doesn't schedule a spurious save. Pop-outs wire separately, as
    // they're created
    void startObserving_()
    {
        connect(
            window_,
            &BaseWindow::geometryChanged,
            this,
            &WorkspaceFile::scheduleSave_);

        connect(
            tree_,
            &TabPaneTree::layoutChanged,
            this,
            &WorkspaceFile::scheduleSave_);

        connect(
            sidebarSplitter_,
            &QSplitter::splitterMoved,
            this,
            &WorkspaceFile::scheduleSave_);

        // Folder expansion. Wired AFTER restore, like the rest here, so
        // restoring expansion doesn't schedule a save; the restore's own
        // programmatic expands are guarded inside VaultFileTree anyway
        if (fileTree_) {
            connect(
                fileTree_,
                &VaultFileTree::expansionChanged,
                this,
                &WorkspaceFile::scheduleSave_);
        }

        if (commonTree_) {
            connect(
                commonTree_,
                &VaultFileTree::expansionChanged,
                this,
                &WorkspaceFile::scheduleSave_);
        }

        // Drawer toggles and tree | drawer handle drags. A toggle resizes
        // through setSizes, which never emits splitterMoved, hence the drawer's
        // own signal
        if (commonDrawer_) {
            connect(
                commonDrawer_,
                &Drawer::stateChanged,
                this,
                &WorkspaceFile::scheduleSave_);
        }
    }

    void scheduleSave_() { debounce_->start(debounceMs_); }

    // Both sidebar trees' expansion as { this, common } — opaque arrays this
    // class places but never reads into. commonTree_ is null for a common-vault
    // window, so only "this" is written there
    [[nodiscard]] QJsonObject serializeExpansion_() const
    {
        QJsonObject expanded{};

        if (fileTree_) {
            expanded[WorkspaceKeys::EXPANDED_THIS] =
                fileTree_->serializeExpansion();
        }

        if (commonTree_) {
            expanded[WorkspaceKeys::EXPANDED_COMMON] =
                commonTree_->serializeExpansion();
        }

        return expanded;
    }

    // Re-expand each tree from its saved array. Runs during restore(), before
    // startObserving_ and before the window is shown; the trees expand
    // synchronously, since their shared model lists any folder on demand. An
    // absent key (older workspace.json, or a common-vault window) yields an
    // empty array and restores nothing
    void restoreExpansion_(const QJsonObject& expanded)
    {
        if (fileTree_) {
            fileTree_->restoreExpansion(
                expanded.value(WorkspaceKeys::EXPANDED_THIS).toArray());
        }

        if (commonTree_) {
            commonTree_->restoreExpansion(
                expanded.value(WorkspaceKeys::EXPANDED_COMMON).toArray());
        }
    }

    // Recreate one pop-out from its persisted entry. VaultWindow makes a
    // hidden, wired pop-out; we restore its geometry and tree, then show it —
    // UNLESS every file it held has since vanished, in which case its tree
    // restores empty and we discard the window rather than float a stray empty
    // pop-out (the tracked destroyed-handler prunes it)
    void restorePopout_(const QJsonObject& node)
    {
        auto tree_node = node.value(WorkspaceKeys::TREE).toObject();
        if (tree_node.isEmpty()) {
            return;
        }

        auto handle = hooks_.createPopout();
        if (!handle.window || !handle.tree) {
            return;
        }

        if (auto geo = node.value(WorkspaceKeys::GEOMETRY).toString();
            !geo.isEmpty()) {
            handle.window->restoreGeometry(
                QByteArray::fromBase64(geo.toLatin1()));
        }

        handle.tree->restore(tree_node, hooks_.buildPage);

        if (handle.tree->hasPages()) {
            handle.window->show();
        } else {
            handle.window->deleteLater();
        }
    }

    [[nodiscard]] QJsonObject serialize_() const
    {
        QJsonObject main{};
        main[WorkspaceKeys::GEOMETRY] =
            QString::fromLatin1(window_->saveGeometry().toBase64());
        main[WorkspaceKeys::SIDEBAR_SIZES] =
            QString::fromLatin1(sidebarSplitter_->saveState().toBase64());
        main[WorkspaceKeys::TREE] = tree_->serialize(hooks_.describePage);
        main[WorkspaceKeys::EXPANDED] = serializeExpansion_();
        if (commonDrawer_) {
            main[WorkspaceKeys::COMMON_DRAWER] =
                commonDrawer_->serializeState();
        }
        main[WorkspaceKeys::RECENT_FILES] = hooks_.describeRecentFiles();

        QJsonObject root{};
        root[WorkspaceKeys::VERSION] = SCHEMA_VERSION_;
        root[WorkspaceKeys::MAIN] = main;
        root[WorkspaceKeys::POPOUTS] = serializePopouts_();
        return root;
    }

    // Each live pop-out as { geometry, tree } — no sidebar, pop-outs have none.
    // A pop-out always holds at least one tab (it auto-closes when emptied), so
    // there's nothing empty to filter here
    [[nodiscard]] QJsonArray serializePopouts_() const
    {
        QJsonArray array{};

        for (const auto& handle : hooks_.enumeratePopouts()) {
            if (!handle.window || !handle.tree) {
                continue;
            }

            QJsonObject entry{};
            entry[WorkspaceKeys::GEOMETRY] =
                QString::fromLatin1(handle.window->saveGeometry().toBase64());
            entry[WorkspaceKeys::TREE] =
                handle.tree->serialize(hooks_.describePage);
            array.append(entry);
        }

        return array;
    }

    // Skipped when the vault folder is gone: ensureVaultDotDir refuses to
    // recreate the root, so there's nowhere to write. CreateDirs::No because
    // .fernanda/ is guaranteed by then — so this write can't recreate the root
    // either, even if it vanishes between the check and the write
    void writeToDisk_()
    {
        if (!ensureVaultDotDir(vaultRoot_)) {
            return;
        }

        JsonIo::write(serialize_(), workspacePath_(), Io::CreateDirs::No);
    }
};

} // namespace Fernanda::Ui
