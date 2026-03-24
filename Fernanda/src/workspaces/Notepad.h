/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026 fairybow
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

#include <QAbstractItemModel>
#include <QDockWidget>
#include <QHash>
#include <QHeaderView>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

#include "core/AppDirs.h"
#include "core/Debug.h"
#include "core/Time.h"
#include "core/Tr.h"
#include "core/Version.h"
#include "fnx/Fnx.h"
#include "menus/MenuBuilder.h"
#include "menus/MenuShortcuts.h"
#include "menus/MenuState.h"
#include "models/AbstractFileModel.h"
#include "services/SettingsService.h"
#include "services/TreeViewService.h"
#include "services/ViewService.h"
#include "services/WindowService.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Backup.h"
#include "workspaces/Bus.h"
#include "workspaces/NotepadFileSystemModel.h"
#include "workspaces/NotepadRecovery.h"
#include "workspaces/SaveFailMessageBox.h"
#include "workspaces/SavePrompt.h"
#include "workspaces/Workspace.h"

/// TODO BA: Notepad autosave and recovery
///
/// Autosave (write):
/// - autosave() override iterates dirty models from files->fileModels()
/// - On-disk files: Io::write(model->data(), recoveryPath) where
///   recoveryPath is AppDirs::tempNotepadRecovery() / {pathHash} / buffer, plus
///   a metadata file (original path, file type, timestamp)
/// - Off-disk files: same pattern under a separate subdirectory with a
/// generated ID instead of pathHash
/// - Bypasses FileService entirely (no watcher suppression, no signals, no
/// modification state changes)
/// - Clean exit deletes the entire tempNotepadRecovery() tree
///
/// Recovery (read):
/// - Detection: check if tempNotepadRecovery() is non-empty in setup_()
/// - On-disk files (original exists): open via files->openFilePathIn(),
/// afterModelCreatedHook sets data from recovery buffer and marks dirty
/// - On-disk files (original missing): treat as off-disk, let user Save As
/// - Off-disk files: open via files->openOffDiskTxtIn(), same hook pattern (set
/// data, mark dirty)
/// - Recovery data deleted after processing
/// - Right now, attempt to restore all. Later, use recovery prompt UI (restore
/// all, selected, or discard?)
///
/// Open questions:
/// - Metadata format (JSON, INI, or simple key=value like NotebookLockfile?)
/// - Off-disk file ID scheme (UUID, counter, hash of content?)

namespace Fernanda {

// A Workspace that operates on the OS filesystem. There is only 1 Notepad
// during the application lifetime
class Notepad : public Workspace
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;

    Notepad(QObject* parent = nullptr)
        : Workspace(parent)
    {
        setup_();
    }

    virtual ~Notepad() override
    {
        TRACER;
        deleteAllRecoveryEntries_(); /// TODO BA
    }

    void openFiles(const Coco::PathList& paths)
    {
        if (auto window = windows->active()) openFiles_(window, paths);
    }

    /// TODO BA
    void recover()
    {
        // NB: If a file is renamed between ticks, the old path-hash entry
        // becomes orphaned until clean exit or recovery

        auto& root = AppDirs::tempNotepadRecovery();
        auto entries = NotepadRecovery::readAll(root);
        if (entries.isEmpty()) return;

        // Separate on-disk (original still exists) from off-disk/orphaned
        QHash<QString, QByteArray> on_disk_buffers{};
        QList<NotepadRecovery::Entry> off_disk_entries{};

        for (auto& entry : entries) {
            if (!entry.isOffDisk() && entry.originalPath.exists())
                on_disk_buffers[entry.originalPath.toQString()] = entry.buffer;
            else
                off_disk_entries << entry;
        }

        // Captures by reference (relies on open calls being synchronous).
        // On-disk entries match by path key. Off-disk entries have no key, so
        // they match by position: takeFirst() pairs them with
        // openOffDiskTxtIn() calls in iteration order
        files->setAfterModelCreatedHook(
            [&on_disk_buffers, &off_disk_entries](AbstractFileModel* model) {
                auto meta = model->meta();
                if (!meta) return;

                if (meta->isOnDisk()) {
                    auto it = on_disk_buffers.find(meta->path().toQString());
                    if (it == on_disk_buffers.end()) return;
                    model->setData(it.value());
                    model->setModified(true);
                    on_disk_buffers.erase(it);
                } else if (!off_disk_entries.isEmpty()) {
                    auto entry = off_disk_entries.takeFirst();
                    model->setData(entry.buffer);
                    model->setModified(true);
                    meta->setTitleOverride(entry.title);
                }
            });

        // Open files (each triggers hook synchronously)
        for (auto& entry : entries) {
            if (!entry.isOffDisk() && entry.originalPath.exists())
                files->openFilePathIn(windows->active(), entry.originalPath);
            else
                files->openOffDiskTxtIn(windows->active());
        }

        files->setAfterModelCreatedHook(nullptr);

        // Clean up recovery data on disk
        for (auto& dir : Coco::paths(root))
            Coco::purge(dir);
    }

    virtual bool tryQuit() override
    {
        return windows->count() < 1 || windows->closeAll();
    }

protected:
    /// TODO BA
    virtual void autosave() override
    {
        TRACER;

        auto& root = AppDirs::tempNotepadRecovery();

        for (auto model : files->fileModels()) {
            if (!model || !model->isModified()) continue;
            auto meta = model->meta();
            if (!meta) continue;

            auto dir = meta->isOnDisk()
                           ? NotepadRecovery::entryDir(root, meta->path())
                           : offDiskRecoveryDir_(root, model);

            NotepadRecovery::write(
                dir,
                meta->path(),
                meta->title(),
                model->data());
            recoveryDirs_[model] = dir;
        }
    }

    virtual QAbstractItemModel* treeViewModel() override { return fsModel_; }

    virtual QModelIndex treeViewRootIndex() override
    {
        // Generate the index on-demand from the stored path (don't hold it
        // separately or retrieve via Model::setRootPath)
        if (!fsModel_) return {};
        return fsModel_->index(currentRootDir.toQString());
    }

    virtual QString treeViewDockIniKey() const override
    {
        return Ini::Keys::NOTEPAD_TREE_VIEW_DOCK;
    }

    virtual bool canCloseTab(Window* window, int index) override
    {
        auto view = views->fileViewAt(window, index);
        if (!view) return false;
        auto model = view->model();
        if (!model) return false;

        // If this model has other views (and so won't be closed with the view),
        // we don't need to worry about saving
        if (!model->isModified() || views->countFor(model) > 1) return true;

        views->raise(window, index);
        auto display_path = fileSaveDisplayPath_(model);

        switch (SavePrompt::exec(display_path, window)) {
        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save:
            switch (singleSave_(model, window)) {
            default:
            case FileService::NoOp:
                return false;
            case FileService::Success:
                colorBars->green(window);
                deleteRecoveryEntry_(model); /// TODO BA
                return true;
            case FileService::Failure:
                colorBars->red(window);
                SaveFailMessageBox::exec(display_path, window);
                return false;
            }

        case SavePrompt::Discard:
            deleteRecoveryEntry_(model); /// TODO BA
            return true;
        }
    }

    virtual bool canCloseTabEverywhere(Window* window, int index) override
    {
        auto view = views->fileViewAt(window, index);
        if (!view) return false;
        auto model = view->model();
        if (!model) return false;

        if (!model->isModified()) return true;

        // Called via menu (on current window + tab), so no need to raise

        auto display_path = fileSaveDisplayPath_(model);

        switch (SavePrompt::exec(display_path, window)) {
        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            switch (singleSave_(model, window)) {
            default:
            case FileService::NoOp:
                return false;
            case FileService::Success:
                colorBars->green(window); // TODO: Could do all windows (close
                                          // everywhere, after all)?
                deleteRecoveryEntry_(model); /// TODO BA
                return true;
            case FileService::Failure:
                colorBars->red(window); // TODO: Could do all windows (close
                                        // everywhere, after all)?
                SaveFailMessageBox::exec(display_path, window);
                return false;
            }
        }

        case SavePrompt::Discard:
            deleteRecoveryEntry_(model); /// TODO BA
            return true;
        }
    }

    virtual bool canCloseWindowTabs(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        auto modified_models = views->modifiedViewModelsIn(
            window,
            ViewService::ExcludeMultiWindow::Yes);
        if (modified_models.isEmpty()) return true;

        auto display_paths = fileSaveDisplayPaths_(modified_models);
        auto prompt_result = SavePrompt::exec(display_paths, window);

        switch (prompt_result.choice) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            // Build list of models to save from selected indices
            QList<AbstractFileModel*> to_save{};
            for (auto& i : prompt_result.selectedIndices)
                to_save << modified_models[i];

            if (to_save.isEmpty()) return true; // Shouldn't happen

            auto result = multiSave_(to_save, window);

            /// TODO BA
            for (auto model : result.succeeded)
                deleteRecoveryEntry_(model);

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red(window);
                auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
                SaveFailMessageBox::exec(fail_display_paths, window);

                return false;
            }

            // If any saves occurred, we indicate that (but still block)
            if (result.aborted) {
                if (result.anySuccesses()) colorBars->green(window);
                return false;
            }

            // All saves succeeded
            colorBars->green(window);
            return true;
        }

        case SavePrompt::Discard:
            /// TODO BA
            for (auto model : modified_models)
                deleteRecoveryEntry_(model);
            return true;
        }
    }

    virtual bool canCloseAllTabs(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return true;

        auto display_paths = fileSaveDisplayPaths_(modified_models);
        auto prompt_result = SavePrompt::exec(
            display_paths,
            windows.last()); // Make top window the dialog owner (top window is
                             // last)

        switch (prompt_result.choice) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            // Build list of models to save from selected indices
            QList<AbstractFileModel*> to_save{};
            for (auto& i : prompt_result.selectedIndices)
                to_save << modified_models[i];

            if (to_save.isEmpty()) return true; // Shouldn't happen

            auto result = multiSave_(to_save);

            /// TODO BA
            for (auto model : result.succeeded)
                deleteRecoveryEntry_(model);

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red();
                auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
                SaveFailMessageBox::exec(fail_display_paths, windows.last());

                return false;
            }

            // If any saves occurred, we indicate that (but still block)
            if (result.aborted) {
                if (result.anySuccesses()) colorBars->green();
                return false;
            }

            // All saves succeeded
            colorBars->green();
            return true;
        }

        case SavePrompt::Discard:
            /// TODO BA
            for (auto model : modified_models)
                deleteRecoveryEntry_(model);
            return true;
        }
    }

    virtual bool canCloseWindow(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        auto modified_models = views->modifiedViewModelsIn(
            window,
            ViewService::ExcludeMultiWindow::Yes);
        if (modified_models.isEmpty()) return true;

        auto display_paths = fileSaveDisplayPaths_(modified_models);
        auto prompt_result = SavePrompt::exec(display_paths, window);

        switch (prompt_result.choice) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            // Build list of models to save from selected indices
            QList<AbstractFileModel*> to_save{};
            for (auto& i : prompt_result.selectedIndices)
                to_save << modified_models[i];

            if (to_save.isEmpty()) return true; // Shouldn't happen

            auto result = multiSave_(to_save, window);

            /// TODO BA
            for (auto model : result.succeeded)
                deleteRecoveryEntry_(model);

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red(window);
                auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
                SaveFailMessageBox::exec(fail_display_paths, window);

                return false;
            }

            // If any saves occurred, we indicate that (but still block)
            if (result.aborted) {
                if (result.anySuccesses()) colorBars->green(window);
                return false;
            }

            // All saves succeeded (no green color bar (window closing))
            return true;
        }

        case SavePrompt::Discard:
            /// TODO BA
            for (auto model : modified_models)
                deleteRecoveryEntry_(model);
            return true;
        }
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return true;

        auto display_paths = fileSaveDisplayPaths_(modified_models);
        auto prompt_result = SavePrompt::exec(
            display_paths,
            windows.last()); // Make top window the dialog owner (top window is
                             // last)

        switch (prompt_result.choice) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            // Build list of models to save from selected indices
            QList<AbstractFileModel*> to_save{};
            for (auto& i : prompt_result.selectedIndices)
                to_save << modified_models[i];

            if (to_save.isEmpty()) return true; // Shouldn't happen

            auto result = multiSave_(to_save);

            /// TODO BA
            for (auto model : result.succeeded)
                deleteRecoveryEntry_(model);

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red();
                auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
                // Use active window, since we may have switched which window is
                // on top?:
                SaveFailMessageBox::exec(fail_display_paths, windows.last());

                return false;
            }

            // If any saves occurred, we indicate that (but still block)
            if (result.aborted) {
                if (result.anySuccesses()) colorBars->green();
                return false;
            }

            // All saves succeeded (no green color bar (window closing))
            return true;
        }

        case SavePrompt::Discard:
            /// TODO BA
            for (auto model : modified_models)
                deleteRecoveryEntry_(model);
            return true;
        }
    }

    virtual void
    fileMenuOpenActions(MenuBuilder& builder, Window* window) override
    {
        if (!window) return;

        builder.action(Tr::npNewTab())
            .onUserTrigger(this, [this, window] { newTab_(window); })
            .shortcut(MenuShortcuts::NEW_TAB)

            .action(Tr::npOpenFile())
            .onUserTrigger(this, [this, window] { promptOpenFiles_(window); })
            .shortcut(MenuShortcuts::OPEN_FILE);
    }

    virtual void fileMenuSaveActions(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) override
    {
        builder.action(Tr::nxSave())
            .onUserTrigger(this, [this, window] { save_(window); })
            .shortcut(MenuShortcuts::SAVE)
            .enabledToggle(
                state,
                MenuScope::ActiveTab,
                [this, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->isModified();
                })

            .action(Tr::nxSaveAs())
            .onUserTrigger(this, [this, window] { saveAs_(window); })
            .shortcut(MenuShortcuts::SAVE_AS)
            .enabledToggle(
                state,
                MenuScope::ActiveTab,
                [this, window] {
                    /// TODO FT: Had to change this after removing
                    /// "supportsEditing" guard in FileService::saveAs. That
                    /// probably means FileService should be the source for this
                    /// query somehow...
                    return views->fileModelAt(window, -1);
                })

            .action(Tr::npSaveAllInWindow())
            .onUserTrigger(this, [this, window] { saveAllInWindow_(window); })
            .enabledToggle(
                state,
                MenuScope::Window,
                [this, window] {
                    return views->anyModifiedFileModelsIn(window);
                })

            .action(Tr::npSaveAll())
            .onUserTrigger(this, [this, window] { saveAll_(window); })
            .shortcut(MenuShortcuts::SAVE_ALL)
            .enabledToggle(state, MenuScope::Workspace, [this] {
                return files->anyModified();
            });
    }

private:
    NotepadFileSystemModel* fsModel_ = new NotepadFileSystemModel(this);
    QHash<AbstractFileModel*, Coco::Path> recoveryDirs_{}; /// TODO BA

    void setup_()
    {
        // Must defer to allow the first window(s) to paint correctly. Without
        // this, QFSM's initialization blocks the event loop (or causes enough
        // strain in any case) long enough to cause white/unpainted windows on
        // startup
        Time::onNextTick([this] {
            fsModel_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
            fsModel_->setRootPath(currentRootDir.toQString());
            fsModel_->setReadOnly(false);

            // Update open file model if it exists
            connect(
                fsModel_,
                &NotepadFileSystemModel::fileRenamed,
                this,
                [this](
                    const QString& path,
                    const QString& oldName,
                    const QString& newName) {
                    if (auto model =
                            files->modelFor(Coco::Path(path) / oldName))
                        model->meta()->setPath(Coco::Path(path) / newName);
                });

            connect(
                fsModel_,
                &NotepadFileSystemModel::fileMoved,
                this,
                [this](const Coco::Path& old, const Coco::Path& now) {
                    if (auto model = files->modelFor(old))
                        model->meta()->setPath(now);
                });
        });

        settings->setName(Tr::notepad());

        treeViews->setHeadersHidden(false);
        treeViews->setDockWidgetHook(this, &Notepad::treeViewDockWidgetHook_);
        treeViews->setVisibilityConfig(
            treeViewDockIniKey(),
            Ini::Defaults::notepadTreeViewDock()); /// TODO TVT

        /// TODO BA
        files->setBeforeWriteHook([](const Coco::Path& path) {
            if (!path.exists())
                return; // Backups don't apply to off-disk files (unlike
                        // recovery/autosave)

            /// TODO BA: Read pruning cap from settings?
            Backup::createAndPrune(path, AppDirs::notepadBackups(), 5);
        });

        connect(
            treeViews,
            &TreeViewService::doubleClicked,
            this,
            &Notepad::onTreeViewsDoubleClicked_);

        connect(
            views,
            &ViewService::addTabRequested,
            this,
            [this](Window* window) { newTab_(window); });

        connect(
            views,
            &ViewService::fileViewDestroyed,
            this,
            [this](AbstractFileView* fileView) {
                if (!fileView) return;
                auto model = fileView->model();
                if (!model || views->countFor(model) > 0) return;
                files->deleteModel(model);
            });

        connectBusEvents_();
    }

    void connectBusEvents_()
    {
        //...
    }

    /// TODO BA: Should this move to NotepadRecovery?
    Coco::Path
    offDiskRecoveryDir_(const Coco::Path& root, AbstractFileModel* model)
    {
        auto it = recoveryDirs_.find(model);
        if (it != recoveryDirs_.end()) return it.value();

        return NotepadRecovery::offDiskEntryDir(root);
    }

    /// TODO BA
    void deleteRecoveryEntry_(AbstractFileModel* model)
    {
        auto it = recoveryDirs_.find(model);
        if (it == recoveryDirs_.end()) return;

        Coco::purge(it.value());
        recoveryDirs_.erase(it);
    }

    /// TODO BA
    void deleteAllRecoveryEntries_()
    {
        for (auto& dir : Coco::paths(AppDirs::tempNotepadRecovery()))
            Coco::purge(dir);

        recoveryDirs_.clear();
    }

    Coco::Path fileSaveDisplayPath_(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return {};
        auto meta = fileModel->meta();
        if (!meta) return {};

        return meta->isOnDisk() ? meta->path()
                                : meta->title() + meta->preferredExt();
    }

    Coco::PathList
    fileSaveDisplayPaths_(const QList<AbstractFileModel*>& fileModels) const
    {
        if (fileModels.isEmpty()) return {};
        Coco::PathList paths{};

        for (auto& model : fileModels)
            if (model) paths << fileSaveDisplayPath_(model);

        return paths;
    }

    FileService::SaveResult
    singleSave_(AbstractFileModel* fileModel, Window* window)
    {
        if (!fileModel || !window) return FileService::NoOp;
        auto meta = fileModel->meta();
        if (!meta) return FileService::NoOp;

        if (meta->isOnDisk() && !meta->isStale())
            return files->save(fileModel);
        else {
            auto path = promptSaveAs_(window, fileModel);
            if (path.isEmpty()) return FileService::NoOp;
            return files->saveAs(fileModel, path);
        }
    }

    struct MultiSaveResult_
    {
        bool aborted = false;
        QList<AbstractFileModel*> succeeded{}; /// TODO BA
        QList<AbstractFileModel*> failed{};

        bool anySuccesses() const noexcept { return !succeeded.isEmpty(); }
        bool anyFails() const noexcept { return !failed.isEmpty(); }
    };

    MultiSaveResult_ multiSave_(
        const QList<AbstractFileModel*>& fileModels,
        Window* window = nullptr)
    {
        if (fileModels.isEmpty()) return {};

        MultiSaveResult_ result{};

        for (auto& model : fileModels) {
            auto meta = model->meta();
            if (!meta) {
                result.failed << model;
                continue;
            }

            FileService::SaveResult save_result{};

            if (meta->isOnDisk()) {
                save_result = files->save(model);
            } else {
                // If window is valid, raise it and then set target_window to
                // that same window. Otherwise, raise the model and set
                // target_window to whatever window raise(model) returns
                auto target_window = window
                                         ? (views->raise(window, model), window)
                                         : views->raise(model);

                if (!target_window) {
                    result.failed << model;
                    continue;
                }

                auto path = promptSaveAs_(target_window, model);

                if (path.isEmpty()) {
                    // User cancelled, abort entire operation
                    result.aborted = true;
                    return result;
                }

                save_result = files->saveAs(model, path);
            }

            switch (save_result) {

            case FileService::Success:
                result.succeeded << model;
                break;

            case FileService::NoOp:
            case FileService::Failure:
            default:
                result.failed << model;
                break;
            }
        }

        return result;
    }

    Coco::Path promptSaveAs_(Window* window, AbstractFileModel* fileModel) const
    {
        if (!window || !fileModel) return {};
        auto meta = fileModel->meta();
        if (!meta) return {};

        Coco::Path start_path =
            meta->isOnDisk()
                ? meta->path()
                : currentRootDir / (meta->title() + meta->preferredExt());

        return Coco::getSaveFile(
            window,
            Tr::npSaveAsCaption(),
            start_path,
            Tr::nxAllFilesFilter()); /// TODO FT
    }

    QWidget* treeViewDockWidgetHook_(TreeView* treeView, Window* window)
    {
        // TODO: Settings or something dynamic based on general dock size
        // settings
        treeView->setColumnWidth(0, 250);
        treeView->header()->moveSection(2, 1);
        return treeView;
    }

    void newTab_(Window* window)
    {
        if (!window) return;
        files->openOffDiskTxtIn(window);
    }

    void promptOpenFiles_(Window* window)
    {
        if (!window) return;

        auto paths = Coco::getFiles(
            window,
            Tr::npOpenFileCaption(),
            rollingOpenStartDir,
            Tr::nxAllFilesFilter()); /// TODO FT
        if (paths.isEmpty()) return;

        rollingOpenStartDir = paths.at(0).parent();
        openFiles_(window, paths);
    }

    void openFiles_(Window* window, const Coco::PathList& paths)
    {
        if (paths.isEmpty()) return;

        for (auto& path : paths) {
            if (!path.exists()) continue;

            Fnx::Io::isFnxFile(path) ? emit openNotebookRequested(path)
                                     : files->openFilePathIn(window, path);
        }
    }

    void save_(Window* window)
    {
        if (!window) return;
        auto current_view = views->fileViewAt(window, -1);
        if (!current_view) return;
        auto model = current_view->model();
        if (!model) return;

        if (!model->isModified()) return;

        // Called via menu (on current window + tab), so no need to raise

        switch (singleSave_(model, window)) {
        default:
        case FileService::NoOp:
            break;
        case FileService::Success:
            colorBars->green(window);
            deleteRecoveryEntry_(model); /// TODO BA
            break;
        case FileService::Failure: {
            colorBars->red(window);
            auto display_path = fileSaveDisplayPath_(model);
            SaveFailMessageBox::exec(display_path, window);
            break;
        }
        }
    }

    void saveAs_(Window* window)
    {
        if (!window) return;
        auto current_view = views->fileViewAt(window, -1);
        if (!current_view) return;
        auto model = current_view->model();
        if (!model) return;

        // Allow Save As on unmodified files!

        /// TODO FT: Removed supportsModification check here
        auto meta = model->meta();
        if (!meta) return;

        // Called via menu (on current window + tab), so no need to
        // raise

        auto path = promptSaveAs_(window, model);
        if (path.isEmpty()) return;

        switch (files->saveAs(model, path)) {
        default:
        case FileService::NoOp:
            break;
        case FileService::Success:
            colorBars->green(window);
            deleteRecoveryEntry_(model); /// TODO BA
            break;
        case FileService::Failure:
            colorBars->red(window);
            auto display_path = fileSaveDisplayPath_(model);
            SaveFailMessageBox::exec(display_path, window);
            break;
        }
    }

    void saveAllInWindow_(Window* window)
    {
        if (!window) return;

        auto modified_models = views->modifiedViewModelsIn(window);
        if (modified_models.isEmpty()) return;

        auto result = multiSave_(modified_models, window);

        /// TODO BA
        for (auto model : result.succeeded)
            deleteRecoveryEntry_(model);

        // Fails take priority
        if (result.anyFails()) {
            colorBars->red(window);
            auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
            SaveFailMessageBox::exec(fail_display_paths, window);

            return;
        }

        // If any saves occurred, we indicate that
        if (result.anySuccesses()) colorBars->green(window);
    }

    void saveAll_(Window* window)
    {
        if (!window) return;

        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return;

        auto result = multiSave_(modified_models);

        /// TODO BA
        for (auto model : result.succeeded)
            deleteRecoveryEntry_(model);

        // Fails take priority
        if (result.anyFails()) {
            colorBars->red();
            auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
            SaveFailMessageBox::exec(fail_display_paths, window);

            return;
        }

        // If any saves occurred, we indicate that
        if (result.anySuccesses()) colorBars->green();
    }

private slots:
    void onTreeViewsDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;

        auto path = Coco::Path(fsModel_->filePath(index));
        if (path.isDir()) return;

        Fnx::Io::isFnxFile(path) ? emit openNotebookRequested(path)
                                 : files->openFilePathIn(window, path);
    }
};

} // namespace Fernanda
