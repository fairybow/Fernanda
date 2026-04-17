/*
 * Fernanda — a plain-text-first workbench for creative writing
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
#include "core/Files.h"
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
#include "settings/Ini.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Backup.h"
#include "workspaces/Bus.h"
#include "workspaces/NotepadFileSystemModel.h"
#include "workspaces/NotepadImport.h"
#include "workspaces/NotepadRecovery.h"
#include "workspaces/SaveFailMessageBox.h"
#include "workspaces/SavePrompt.h"
#include "workspaces/Workspace.h"

namespace Fernanda {

// A Workspace that operates on the OS filesystem. There is only 1 Notepad
// during the application lifetime
class Notepad : public Workspace
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;

    Notepad(QObject* parent = nullptr)
        : Workspace(
              { Ini::LocalKeys::NOTEPAD_TREE_VIEW_DOCK,
                Ini::LocalKeys::NOTEPAD_UNIQUE_TABS },
              parent)
    {
        setup_();
    }

    virtual ~Notepad() override
    {
        TRACER;
        deleteAllRecoveryEntries_(); /// TODO BA
    }

        virtual bool tryQuit() override
    {
        return windows->count() < 1 || windows->closeAll();
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
        // openOffDiskPlainTextFileIn() calls in iteration order
        files->setAfterModelCreatedHook(
            [this, &root, &on_disk_buffers, &off_disk_entries](
                AbstractFileModel* model) {
                auto meta = model->meta();
                if (!meta) return;

                if (meta->isOnDisk()) {
                    auto it = on_disk_buffers.find(meta->path().toQString());
                    if (it == on_disk_buffers.end()) return;
                    model->setData(it.value());
                    model->setModified(true);
                    on_disk_buffers.erase(it);
                    recoveryDirs_[model] =
                        NotepadRecovery::entryDir(root, meta->path());
                } else if (!off_disk_entries.isEmpty()) {
                    auto entry = off_disk_entries.takeFirst();
                    model->setData(entry.buffer);
                    model->setModified(true);
                    meta->setTitleOverride(entry.title);
                    recoveryDirs_[model] = entry.entryDir;
                }
            });

        auto window = windows->active();
        if (!window) return;

        // Open files (each triggers hook synchronously)
        for (auto& entry : entries) {
            if (!entry.isOffDisk() && entry.originalPath.exists()) {
                files->openFilePathIn(window, entry.originalPath);
            } else {
                files->openOffDiskPlainTextFileIn(window, entry.fileType);
            }
        }

        files->setAfterModelCreatedHook(nullptr);

        // Recovery entries stay on disk rather than being purged, so a crash
        // before the next autosave tick doesn't lose dirty data. The
        // recoveryDirs_ map connects each model to its entry, so save, discard,
        // and undo-to-clean remove them normally

        // Sanity: hook should have consumed everything it was given
        ASSERT(on_disk_buffers.isEmpty());
        // off_disk_entries may not be empty if some openOffDiskPlainTextFileIn
        // calls failed to create models, but that would indicate a deeper
        // problem
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

            auto dir = ensureRecoveryDir_(root, model);
            if (dir.isEmpty()) continue;

            NotepadRecovery::write(
                dir,
                meta->path(),
                meta->title(),
                meta->fileType(),
                model->data());
        }
    }

    /// TODO NF
    virtual void newFile(Window* window, Files::Type fileType) override
    {
        newTab_(window, fileType);
    }

    /// TODO NF
    virtual void
    importFiles(Window* window, const Coco::PathList& paths) override
    {
        auto results = NotepadImport::process(paths);

        for (const auto& result : results) {
            files->openOffDiskPlainTextFileIn(
                window,
                result.type,
                result.suggestedName,
                result.text);
        }
    }

    /// TODO NF
    virtual QString importFilter() const override
    {
        return Files::conversionImportsFilter();
    }

    virtual QAbstractItemModel* treeViewModel() override { return fsModel_; }

    virtual QModelIndex treeViewRootIndex() override
    {
        // Generate the index on-demand from the stored path (don't hold it
        // separately or retrieve via Model::setRootPath)
        if (!fsModel_) return {};
        return fsModel_->index(currentRootDir.toQString());
    }

    /// TODO TS
    virtual bool canCloseTab(Window* window, AbstractFileModel* model) override
    {
        if (!model) return false;
        if (!model->isModified() || views->countFor(model) > 1) return true;

        views->raise(model);
        return promptSingleModelClosingSave_(model, window);
    }

    /// TODO TS
    virtual bool
    canCloseTabEverywhere(Window* window, AbstractFileModel* model) override
    {
        if (!model) return false;
        if (!model->isModified()) return true;

        return promptSingleModelClosingSave_(model, window);
    }

    /// TODO TS
    virtual bool canCloseSplit(Window* window) override
    {
        auto modified = views->modifiedViewModelsInActiveSplit(window);
        if (modified.isEmpty()) return true;

        return promptMultiModelClosingSave_(modified, window, window, true);
    }

    virtual bool canCloseWindowTabs(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        auto modified = views->modifiedViewModelsIn(
            window,
            ViewService::ExcludeMultiWindow::Yes);
        if (modified.isEmpty()) return true;

        return promptMultiModelClosingSave_(modified, window, window, true);
    }

    virtual bool canCloseAllTabs(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified = views->modifiedViewModels();
        if (modified.isEmpty()) return true;

        return promptMultiModelClosingSave_(
            modified,
            windows.last(),
            nullptr,
            true);
    }

    virtual bool canCloseWindow(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        auto modified = views->modifiedViewModelsIn(
            window,
            ViewService::ExcludeMultiWindow::Yes);
        if (modified.isEmpty()) return true;

        return promptMultiModelClosingSave_(modified, window, window, false);
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified = views->modifiedViewModels();
        if (modified.isEmpty()) return true;

        return promptMultiModelClosingSave_(
            modified,
            windows.last(),
            nullptr,
            false);
    }

    virtual void
    fileMenuOpenActions(MenuBuilder& builder, Window* window) override
    {
        if (!window) return;

        builder.action(Tr::npOpenFile())
            .onUserTrigger(this, [this, window] { promptOpenFiles_(window); })
            .shortcut(MenuShortcuts::OPEN_FILE);
    }

    virtual void fileMenuSaveActions(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) override
    {
        builder.action(Tr::nxSave())
            .onUserTrigger(this, [this, window] { save_(window, -1); })
            .shortcut(MenuShortcuts::SAVE)
            .enabledToggle(
                state,
                MenuScope::ActiveTab,
                [this, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->isModified();
                })

            .action(Tr::nxSaveAs())
            .onUserTrigger(this, [this, window] { saveAs_(window, -1); })
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

    virtual void tabContextMenuSaveActions(
        MenuBuilder& builder,
        Window* window,
        int index) override
    {
        if (!window) return;
        auto model = views->fileModelAt(window, index);
        if (!model) return;

        builder.actionIf(model->isModified(), Tr::nxSave())
            .onUserTrigger(
                this,
                [this, window, index] { save_(window, index); })

            .action(Tr::nxSaveAs())
            .onUserTrigger(this, [this, window, index] {
                saveAs_(window, index);
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
        /// TODO BA
        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            [this](AbstractFileModel* fileModel, bool modified) {
                if (!modified) deleteRecoveryEntry_(fileModel);
            });
    }

    /// TODO BA
    Coco::Path
    ensureRecoveryDir_(const Coco::Path& root, AbstractFileModel* model)
    {
        auto it = recoveryDirs_.find(model);
        if (it != recoveryDirs_.end()) return it.value();

        auto meta = model->meta();
        if (!meta) return {};

        auto dir = meta->isOnDisk()
                       ? NotepadRecovery::entryDir(root, meta->path())
                       : NotepadRecovery::offDiskEntryDir(root);

        recoveryDirs_[model] = dir;
        return dir;
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

    bool promptSingleModelClosingSave_(AbstractFileModel* model, Window* window)
    {
        if (!model || !window) return false;

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

            case FileService::Failure:
                colorBars->red(
                    window); // TODO: See note on colorBars->green above
                SaveFailMessageBox::exec(display_path, window);
                return false;

            case FileService::Success:
                colorBars->green(
                    window); // TODO: Could do all windows somehow, when the
                             // saved file is in multiple windows (one in each
                             // window containing the file (would need to track
                             // that or gather it before calling this and always
                             // pass a list
            }

            [[fallthrough]]; // Only Success reaches here (inner switch's other
                             // branches return)
        }

        case SavePrompt::Discard:
            deleteRecoveryEntry_(model); /// TODO BA
            return true;
        }
    }

    bool promptMultiModelClosingSave_(
        const QList<AbstractFileModel*>& modifiedModels,
        Window* dialogOwner,
        Window* saveTargetWindow,
        bool showGreenOnSuccess)
    {
        auto display_paths = fileSaveDisplayPaths_(modifiedModels);
        auto prompt_result = SavePrompt::exec(display_paths, dialogOwner);

        switch (prompt_result.choice) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            // Build list of models to save from selected indices
            QList<AbstractFileModel*> to_save{};
            for (auto& i : prompt_result.selectedIndices)
                to_save << modifiedModels[i];

            if (to_save.isEmpty()) return true; // Shouldn't happen

            auto result = multiSave_(to_save, saveTargetWindow);

            /// TODO BA
            // (See end of this case)
            for (auto model : result.succeeded)
                deleteRecoveryEntry_(model);

            // Fails take priority
            if (result.anyFails()) {
                saveTargetWindow ? colorBars->red(saveTargetWindow)
                                 : colorBars->red();
                auto fail_display_paths = fileSaveDisplayPaths_(result.failed);
                SaveFailMessageBox::exec(fail_display_paths, dialogOwner);

                return false;
            }

            // If any saves occurred, we indicate that (but still block)
            if (result.aborted) {
                if (result.anySuccesses()) {
                    saveTargetWindow ? colorBars->green(saveTargetWindow)
                                     : colorBars->green();
                }

                return false;
            }

            // All saves succeeded
            if (showGreenOnSuccess) {
                saveTargetWindow ? colorBars->green(saveTargetWindow)
                                 : colorBars->green();
            }

            // NB: No fallthrough, since we have to handle recovery autosave
            // cleanup differently. Save deletes recovery entries only for
            // succeeded models (and must do so before a possible fail/abort
            // return), while Discard deletes them for all modifiedModels
            return true;
        }

        case SavePrompt::Discard:
            /// TODO BA
            for (auto model : modifiedModels)
                deleteRecoveryEntry_(model);
            return true;
        }
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
            Files::filters(Files::All)); /// TODO NF
    }

    // TODO: Remove window parameter?
    QWidget*
    treeViewDockWidgetHook_(TreeView* treeView, [[maybe_unused]] Window* window)
    {
        // TODO: Settings or something dynamic based on general dock size
        // settings
        treeView->setColumnWidth(0, 250);
        treeView->header()->moveSection(2, 1);
        return treeView;
    }

    /// TODO NF: Make plainTextFileType required param?
    void
    newTab_(Window* window, Files::Type plainTextFileType = Files::PlainText)
    {
        if (!window) return;
        files->openOffDiskPlainTextFileIn(window, plainTextFileType);
    }

    void promptOpenFiles_(Window* window)
    {
        if (!window) return;

        auto paths = Coco::getFiles(
            window,
            Tr::npOpenFileCaption(),
            rollingOpenStartDir,
            Files::filters(Files::All)); /// TODO NF
        if (paths.isEmpty()) return;

        rollingOpenStartDir = paths.at(0).parent();
        openFiles_(window, paths);
    }

    void openFiles_(Window* window, const Coco::PathList& paths)
    {
        if (paths.isEmpty()) return;

        for (auto& path : paths) {
            if (!path.exists()) continue;

            Files::isFnxFile(path) ? emit openNotebookRequested(path)
                                   : files->openFilePathIn(window, path);
        }
    }

    void save_(Window* window, int index)
    {
        if (!window) return;

        auto model = views->fileModelAt(window, index);
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

    void saveAs_(Window* window, int index)
    {
        if (!window) return;

        auto model = views->fileModelAt(window, index);
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
        for (auto model : result.succeeded) {
            deleteRecoveryEntry_(model);
        }

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

        Files::isFnxFile(path) ? emit openNotebookRequested(path)
                               : files->openFilePathIn(window, path);
    }
};

} // namespace Fernanda
