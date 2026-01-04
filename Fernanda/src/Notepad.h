/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QDockWidget>
#include <QFileSystemModel>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "AppDirs.h"
#include "Bus.h"
#include "Debug.h"
#include "Fnx.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "SaveFailMessageBox.h"
#include "SavePrompt.h"
#include "SettingsService.h"
#include "Tr.h"
#include "TreeViewService.h"
#include "Timers.h"
#include "Version.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"
#include "Workspace.h"

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

    virtual ~Notepad() override { TRACER; }

    void openFiles(const QList<Coco::Path>& paths)
    {
        if (auto window = windows->active()) openFiles_(window, paths);
    }

    virtual bool tryQuit() override
    {
        return windows->count() < 1 || windows->closeAll();
    }

protected:
    virtual QAbstractItemModel* treeViewModel() override { return fsModel_; }

    virtual QModelIndex treeViewRootIndex() override
    {
        // Generate the index on-demand from the stored path (don't hold it
        // separately or retrieve via Model::setRootPath)
        if (!fsModel_) return {};
        return fsModel_->index(startDir.toQString());
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
        auto name = fileDisplayName_(model);

        switch (SavePrompt::exec(name, window)) {
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
                return true;
            case FileService::Failure:
                colorBars->red(window);
                SaveFailMessageBox::exec(name, window);
                return false;
            }

        case SavePrompt::Discard:
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

        auto name = fileDisplayName_(model);

        switch (SavePrompt::exec(name, window)) {
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
                return true;
            case FileService::Failure:
                colorBars->red(window); // TODO: Could do all windows (close
                                        // everywhere, after all)?
                SaveFailMessageBox::exec(name, window);
                return false;
            }
        }

        case SavePrompt::Discard:
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

        auto display_names = fileDisplayNames_(modified_models);
        auto prompt_result = SavePrompt::exec(display_names, window);

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

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red(window);
                auto fail_display_names = fileDisplayNames_(result.failed);
                SaveFailMessageBox::exec(fail_display_names, window);

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
            return true;
        }
    }

    virtual bool canCloseAllTabs(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return true;

        auto display_names = fileDisplayNames_(modified_models);
        auto prompt_result = SavePrompt::exec(
            display_names,
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

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red();
                auto fail_display_names = fileDisplayNames_(result.failed);
                SaveFailMessageBox::exec(fail_display_names, windows.last());

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

        auto display_names = fileDisplayNames_(modified_models);
        auto prompt_result = SavePrompt::exec(display_names, window);

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

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red(window);
                auto fail_display_names = fileDisplayNames_(result.failed);
                SaveFailMessageBox::exec(fail_display_names, window);

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
            return true;
        }
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return true;

        auto display_names = fileDisplayNames_(modified_models);
        auto prompt_result = SavePrompt::exec(
            display_names,
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

            // Fails take priority
            if (result.anyFails()) {
                colorBars->red();
                auto fail_display_names = fileDisplayNames_(result.failed);
                // Use active window, since we may have switched which window is
                // on top?:
                SaveFailMessageBox::exec(fail_display_names, windows.last());

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
            return true;
        }
    }

    virtual void
    fileMenuOpenActions(MenuBuilder& builder, Window* window) override
    {
        if (!window) return;

        builder.action(Tr::npNewTab())
            .slot(this, [&, window] { newTab_(window); })
            .shortcut(MenuShortcuts::NEW_TAB)

            .action(Tr::npOpenFile())
            .slot(this, [&, window] { promptOpenFiles_(window); })
            .shortcut(MenuShortcuts::OPEN_FILE);
    }

    virtual void fileMenuSaveActions(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) override
    {
        builder.action(Tr::nxSave())
            .slot(this, [&, window] { save_(window); })
            .shortcut(MenuShortcuts::SAVE)
            .toggle(
                state,
                MenuScope::ActiveTab,
                [&, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->isModified();
                })

            .action(Tr::nxSaveAs())
            .slot(this, [&, window] { saveAs_(window); })
            .shortcut(MenuShortcuts::SAVE_AS)
            .toggle(
                state,
                MenuScope::ActiveTab,
                [&, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->supportsModification();
                })

            .action(Tr::npSaveAllInWindow())
            .slot(this, [&, window] { saveAllInWindow_(window); })
            .toggle(
                state,
                MenuScope::Window,
                [&, window] { return views->anyModifiedFileModelsIn(window); })

            .action(Tr::npSaveAll())
            .slot(this, [&, window] { saveAll_(window); })
            .shortcut(MenuShortcuts::SAVE_ALL)
            .toggle(state, MenuScope::Workspace, [&] {
                return files->anyModified();
            });
    }

private:
    QFileSystemModel* fsModel_ = new QFileSystemModel(this);

    void setup_()
    {
        // Must defer to allow the first window(s) to paint correctly. Without
        // this, QFSM's initialization blocks the event loop (or causes enough
        // strain in any case) long enough to cause white/unpainted windows on
        // startup
        Timers::onNextTick([&] {
            fsModel_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
            fsModel_->setRootPath(startDir.toQString());
        });

        settings->setName(Tr::notepad());

        treeViews->setHeadersHidden(false);
        treeViews->setDockWidgetFeatures(
            QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

        connect(
            treeViews,
            &TreeViewService::doubleClicked,
            this,
            &Notepad::onTreeViewsDoubleClicked_);

        connect(
            views,
            &ViewService::viewDestroyed,
            this,
            [&](AbstractFileModel* fileModel) {
                if (!fileModel || views->countFor(fileModel) > 0) return;
                files->deleteModel(fileModel);
            });

        connect(
            views,
            &ViewService::addTabRequested,
            this,
            [&](Window* window) { newTab_(window); });

        connectBusEvents_();
    }

    void connectBusEvents_()
    {
        //...
    }

    QString fileDisplayName_(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return {};
        auto meta = fileModel->meta();
        if (!meta) return {};

        auto path = meta->path();
        return path.isEmpty() ? meta->title() + fileModel->preferredExtension()
                              : path.toQString();
    }

    QStringList
    fileDisplayNames_(const QList<AbstractFileModel*>& fileModels) const
    {
        if (fileModels.isEmpty()) return {};
        QStringList names{};

        for (auto& model : fileModels)
            if (model) names << fileDisplayName_(model);

        return names;
    }

    FileService::SaveResult
    singleSave_(AbstractFileModel* fileModel, Window* window)
    {
        if (!fileModel || !window) return FileService::NoOp;
        auto meta = fileModel->meta();
        if (!meta) return FileService::NoOp;

        if (meta->isOnDisk())
            return files->save(fileModel);
        else {
            auto path = promptSaveAs_(window, fileModel);
            if (path.isEmpty()) return FileService::NoOp;
            return files->saveAs(fileModel, path);
        }
    }

    struct MultiSaveResult_
    {
        int successCount = 0;
        bool aborted = false;
        QList<AbstractFileModel*> failed{};

        bool anySuccesses() const noexcept { return successCount > 0; }
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
                ++result.successCount;
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

        auto path = meta->path();
        Coco::Path start_path =
            path.isEmpty()
                ? startDir / (meta->title() + fileModel->preferredExtension())
                : path;

        return Coco::PathUtil::Dialog::save(
            window,
            Tr::npSaveAsCaption(),
            start_path,
            Tr::npSaveAsFilter());
    }

    void newTab_(Window* window)
    {
        if (!window) return;
        files->openOffDiskTxtIn(window);
    }

    void promptOpenFiles_(Window* window)
    {
        if (!window) return;

        auto paths = Coco::PathUtil::Dialog::files(
            window,
            Tr::npOpenFileCaption(),
            startDir,
            Tr::npOpenFileFilter());

        openFiles_(window, paths);
    }

    void openFiles_(Window* window, const QList<Coco::Path>& paths)
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
            break;
        case FileService::Failure: {
            colorBars->red(window);
            auto name = fileDisplayName_(model);
            SaveFailMessageBox::exec(name, window);
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
        if (!model->supportsModification()) return;
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
            break;
        case FileService::Failure:
            colorBars->red(window);
            auto name = fileDisplayName_(model);
            SaveFailMessageBox::exec(name, window);
            break;
        }
    }

    void saveAllInWindow_(Window* window)
    {
        if (!window) return;

        auto modified_models = views->modifiedViewModelsIn(window);
        if (modified_models.isEmpty()) return;

        auto result = multiSave_(modified_models, window);

        // Fails take priority
        if (result.anyFails()) {
            colorBars->red(window);
            auto fail_display_names = fileDisplayNames_(result.failed);
            SaveFailMessageBox::exec(fail_display_names, window);

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

        // Fails take priority
        if (result.anyFails()) {
            colorBars->red();
            auto fail_display_names = fileDisplayNames_(result.failed);
            SaveFailMessageBox::exec(fail_display_names, window);

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
        if (path.isFolder()) return;

        Fnx::Io::isFnxFile(path) ? emit openNotebookRequested(path)
                                 : files->openFilePathIn(window, path);
    }
};

} // namespace Fernanda
