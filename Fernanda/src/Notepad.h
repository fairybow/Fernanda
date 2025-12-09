/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QAbstractItemModel>
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
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "IService.h"
#include "NotepadMenuModule.h"
#include "SaveFailMessageBox.h"
#include "SavePrompt.h"
#include "TreeViewService.h"
#include "Version.h"
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

    DECLARE_HOOK_ACCESSORS(
        PathInterceptor,
        pathInterceptor,
        setPathInterceptor,
        pathInterceptor_);

    bool hasWindows() const { return windows->count() > 0; }

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
        return fsModel_->index(currentBaseDir_.toQString());
    }

    virtual void newTab(Window* window) override
    {
        if (!window) return;
        files->openOffDiskTxtIn(window);
    }

    /// TODO SAVES: Once save operations are implemented, factor out common
    /// patterns (in closures here and commands below):
    /// - Modified model collection (single window vs all windows, with/without
    /// multi-window filter)
    /// - Display name extraction from FileMeta (also add FileMeta preferred
    /// extension)
    /// - SavePrompt switch handling

    virtual bool canCloseTab(Window* window, int index) override
    {
        auto view = views->fileViewAt(window, index);
        if (!view) return false;
        auto model = view->model();
        if (!model) return false;
        auto meta = model->meta();
        if (!meta) return false;

        // If this model has other views (and so won't be closed with the view),
        // we don't need to worry about saving
        if (!model->isModified() || views->countFor(model) > 1) return true;

        auto name = fileDisplayName_(model);

        switch (SavePrompt::exec(name, window)) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {

            FileService::SaveResult result{};

            if (meta->isOnDisk()) {
                result = files->save(model);
            } else {
                // Off-disk: need Save As dialog
                views->raise(window, index);

                // If dialog is aborted, return false to block hook caller
                // and show red color bar feedback
                auto path = Coco::PathUtil::Dialog::save(
                    window,
                    Tr::Dialogs::notepadSaveFileAsCaption(),
                    currentBaseDir_);

                if (path.isEmpty()) return false;

                result = files->saveAs(model, path);
            }

            if (result == FileService::Success) {
                colorBars->green(window);
                return true;
            } else {
                colorBars->red(window);
                SaveFailMessageBox::exec(name, window);
                return false;
            }
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
        auto meta = model->meta();
        if (!meta) return false;

        if (!model->isModified()) return true;

        auto name = fileDisplayName_(model);

        switch (SavePrompt::exec(name, window)) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {

            FileService::SaveResult result{};

            if (meta->isOnDisk()) {
                result = files->save(model);
            } else {
                // Off-disk: need Save As dialog

                // Close Tab Everywhere is currently only called via menu (so on
                // the current tab), so this is not techincally needed
                views->raise(window, index);

                // If dialog is aborted, return false to block hook caller
                // and show red color bar feedback
                auto path = Coco::PathUtil::Dialog::save(
                    window,
                    Tr::Dialogs::notepadSaveFileAsCaption(),
                    currentBaseDir_);

                if (path.isEmpty()) return false;

                result = files->saveAs(model, path);
            }

            if (result == FileService::Success) {
                colorBars->green(window);
                return true;
            } else {
                colorBars->red(window);
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
        QList<AbstractFileModel*> modified_models{};

        for (auto& view : views->fileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;
            if (modified_models.contains(model)) continue;

            modified_models << model;
        }

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

            /// TODO SAVES: Good (I think) - mirror in others:

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
        QList<AbstractFileModel*> modified_models{};

        for (auto& window : windows) {
            for (auto& view : views->fileViewsIn(window)) {
                if (!view) continue;
                auto model = view->model();
                if (!model || !model->isModified()) continue;
                if (modified_models.contains(model)) continue;

                modified_models << model;
            }
        }

        if (modified_models.isEmpty()) return true;

        auto display_names = fileDisplayNames_(modified_models);

        // Make top window the dialog owner (top window is last)
        auto prompt_result = SavePrompt::exec(display_names, windows.last());

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
        QList<AbstractFileModel*> modified_models{};

        for (auto& view : views->fileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;
            if (modified_models.contains(model)) continue;

            modified_models << model;
        }

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
        QList<AbstractFileModel*> modified_models{};

        for (auto& window : windows) {
            for (auto& view : views->fileViewsIn(window)) {
                if (!view) continue;
                auto model = view->model();
                if (!model || !model->isModified()) continue;
                if (modified_models.contains(model)) continue;

                modified_models << model;
            }
        }

        if (modified_models.isEmpty()) return true;

        auto display_names = fileDisplayNames_(modified_models);

        // Make top window the dialog owner (top window is last)
        auto prompt_result = SavePrompt::exec(display_names, windows.last());

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

private:
    Coco::Path currentBaseDir_ = AppDirs::defaultDocs();
    PathInterceptor pathInterceptor_ = nullptr;
    QFileSystemModel* fsModel_ = new QFileSystemModel(this);
    NotepadMenuModule* menus_ = new NotepadMenuModule(bus, this);

    /// TODO SAVES

    /// Remember, we want the following re: Save As dialogs:
    /// - If user cancels dialog, that isn't itself a failure
    /// - If anything was saved (and nothing failed) before canceling, we show
    /// green
    /// - If anything failed before canceling, we show red and fail prompt
    /// - If nothing failed or succeeded before canceling, we show nothing
    /// - For single file, Save As: canceled, no color bar; success green; fail
    /// red
    /// - For single file save: shouldn't be any opportunity for no color bar;
    /// red fail; green success (obvs) Go through every save method again and
    /// make sure we're following the above

    struct MultiSaveResult_
    {
        int successCount = 0;
        bool aborted = false;
        QList<AbstractFileModel*> failed{};

        bool anySuccesses() const noexcept { return successCount > 0; }
        bool anyFails() const noexcept { return !failed.isEmpty(); }
    };

    QString fileDisplayName_(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return {};
        auto meta = fileModel->meta();
        if (!meta) return {};

        // TODO: Add a preferred extension so off-disk files can say
        // "TempTitle.txt"
        return meta->path().isEmpty() ? meta->title()
                                      : meta->path().toQString();
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

    MultiSaveResult_ multiSave_(
        const QList<AbstractFileModel*>& fileModels,
        Window* window = nullptr)
    {
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
                // Raise tab before showing Save As dialog

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

                auto path = Coco::PathUtil::Dialog::save(
                    target_window,
                    Tr::Dialogs::notepadSaveFileAsCaption(),
                    currentBaseDir_);

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

    /// TODO SAVES (END)

    void setup_()
    {
        // Via Qt: Setting root path installs a filesystem watcher
        fsModel_->setRootPath(currentBaseDir_.toQString());
        fsModel_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

        menus_->initialize();

        //...

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        bus->addCommandHandler(
            Commands::NOTEPAD_OPEN_FILE,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                auto paths = Coco::PathUtil::Dialog::files(
                    cmd.context,
                    Tr::Dialogs::notepadOpenFileCaption(),
                    currentBaseDir_,
                    Tr::Dialogs::notepadOpenFileFilter());

                if (paths.isEmpty()) return;

                for (auto& path : paths) {
                    if (!path.exists()) continue;
                    // TODO: Still calling this here so we have the path
                    // filtered by the interceptor (to open .fnx)!
                    bus->execute(
                        Commands::OPEN_FILE_AT_PATH,
                        { { "path", qVar(path) } },
                        cmd.context);
                }
            });

        /// TODO SAVES

        bus->addCommandHandler(Commands::NOTEPAD_SAVE, [&](const Command& cmd) {
            if (!cmd.context) return;
            auto current_view = views->fileViewAt(cmd.context, -1);
            if (!current_view) return;
            auto model = current_view->model();
            if (!model || !model->supportsModification()) return;
            if (!model->isModified()) return;

            auto meta = model->meta();
            if (!meta) return;

            FileService::SaveResult result{};

            if (meta->isOnDisk()) {
                result = files->save(model);
            } else {
                // Off-disk: need Save As dialog

                // TODO: Pretty sure this only ever hits the current
                // window/view, so we don't need to raise for Save As

                auto path = Coco::PathUtil::Dialog::save(
                    cmd.context,
                    Tr::Dialogs::notepadSaveFileAsCaption(),
                    currentBaseDir_); // TODO: For this and similar, want to add
                                      // the current temp title + preferred ext

                if (path.isEmpty()) return;

                result = files->saveAs(model, path);
            }

            if (result == FileService::Success) {
                colorBars->green(cmd.context);
            } else {
                colorBars->red(cmd.context);
                auto name = fileDisplayName_(model);
                SaveFailMessageBox::exec(name, cmd.context);
            }
        });

        bus->addCommandHandler(
            Commands::NOTEPAD_SAVE_AS,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                auto current_view = views->fileViewAt(cmd.context, -1);
                if (!current_view) return;
                auto model = current_view->model();
                if (!model || !model->supportsModification()) return;

                auto meta = model->meta();
                if (!meta) return;

                // Allow Save As on unmodified files!

                auto initial_path =
                    meta->isOnDisk() ? meta->path() : currentBaseDir_;

                // TODO: Pretty sure this only ever hits the current
                // window/view, so we don't need to raise for Save As

                auto path = Coco::PathUtil::Dialog::save(
                    cmd.context,
                    Tr::Dialogs::notepadSaveFileAsCaption(),
                    initial_path);

                if (path.isEmpty()) return;

                auto result = files->saveAs(model, path);

                if (result == FileService::Success) {
                    colorBars->green(cmd.context);
                } else {
                    colorBars->red(cmd.context);
                    auto name = fileDisplayName_(model);
                    SaveFailMessageBox::exec(name, cmd.context);
                }
            });

        bus->addCommandHandler(
            Commands::NOTEPAD_SAVE_ALL_IN_WINDOW,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                QList<AbstractFileModel*> modified_models{};
                for (auto& view : views->fileViewsIn(cmd.context)) {
                    if (!view) continue;
                    auto model = view->model();
                    if (!model || !model->isModified()) continue;
                    if (modified_models.contains(model)) continue;

                    modified_models << model;
                }

                if (modified_models.isEmpty()) return;

                auto result = multiSave_(modified_models, cmd.context);

                // Fails take priority
                if (result.anyFails()) {
                    colorBars->red(cmd.context);
                    auto fail_display_names = fileDisplayNames_(result.failed);
                    SaveFailMessageBox::exec(fail_display_names, cmd.context);

                    return;
                }

                // If any saves occurred, we indicate that
                if (result.anySuccesses()) colorBars->green(cmd.context);
            });

        bus->addCommandHandler(
            Commands::NOTEPAD_SAVE_ALL,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                QList<AbstractFileModel*> modified_models{};

                for (auto& window : windows->windows()) {
                    // TODO: Could use views->fileViews instead of getting all
                    // windows - sorta doesn't matter
                    for (auto& view : views->fileViewsIn(window)) {
                        if (!view) continue;
                        auto model = view->model();
                        if (!model || !model->isModified()) continue;
                        if (modified_models.contains(model)) continue;

                        modified_models << model;
                    }
                }

                if (modified_models.isEmpty()) return;

                auto result = multiSave_(modified_models);

                // Fails take priority
                if (result.anyFails()) {
                    colorBars->red();
                    auto fail_display_names = fileDisplayNames_(result.failed);
                    SaveFailMessageBox::exec(fail_display_names, cmd.context);

                    return;
                }

                // If any saves occurred, we indicate that
                if (result.anySuccesses()) colorBars->green();
            });

        /// TODO SAVES (END)

        // Notepad sets an Interceptor for FileService's open file command in
        // order to intercept Notebook paths
        bus->addInterceptor(
            Commands::OPEN_FILE_AT_PATH,
            [&](const Command& cmd) {
                if (pathInterceptor_
                    && pathInterceptor_(cmd.param<Coco::Path>("path")))
                    return true;

                return false;
            });
    }

    void connectBusEvents_()
    {
        connect(
            bus,
            &Bus::treeViewDoubleClicked,
            this,
            &Notepad::onTreeViewDoubleClicked_);

        connect(bus, &Bus::viewDestroyed, this, &Notepad::onViewDestroyed_);
    }

private slots:
    void onTreeViewDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;

        auto path = Coco::Path(fsModel_->filePath(index));
        if (path.isFolder()) return;

        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(path) } },
            window);
    }

    void onViewDestroyed_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;
        if (views->countFor(fileModel) > 0) return;

        files->deleteModel(fileModel);
    }
};

} // namespace Fernanda
