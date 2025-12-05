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

#include "AppDirs.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "IService.h"
#include "NotepadMenuModule.h"
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

    virtual bool canQuit() override
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

    virtual bool canCloseTab(Window* window, int index) override
    {
        auto view = views->fileViewAt(window, index);
        if (!view) return false;
        auto model = view->model();
        if (!model) return false;
        auto meta = model->meta();
        if (!meta) return false;

        if (model->isModified() && views->countFor(model) <= 1) {
            views->raise(window, index);

            // TODO: Add a preferred extension so off-disk files can say
            // "TempTitle.txt"
            auto name = meta->path().isEmpty() ? meta->title()
                                               : meta->path().toQString();

            switch (SavePrompt::exec(name, window)) {
            case SavePrompt::Cancel:
                return false;
            case SavePrompt::Save:
                // TODO: Save, once we've moved it to FileService
                return true;
            case SavePrompt::Discard:
                return true;
            }
        }

        return true;
    }

    virtual bool canCloseTabEverywhere(Window* window, int index) override
    {
        auto view = views->fileViewAt(window, index);
        if (!view) return false;
        auto model = view->model();
        if (!model) return false;
        auto meta = model->meta();
        if (!meta) return false;

        if (model->isModified()) {
            // Close Tab Everywhere is currently only called via menu (so on the
            // current tab), so this is not techincally needed
            views->raise(window, index);

            auto name = meta->path().isEmpty() ? meta->title()
                                               : meta->path().toQString();

            switch (SavePrompt::exec(name, window)) {
            case SavePrompt::Cancel:
                return false;
            case SavePrompt::Save:
                // TODO: Save, once we've moved it to FileService
                return true;
            case SavePrompt::Discard:
                return true;
            }
        }

        return true;
    }

    virtual bool canCloseWindowTabs(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        QList<IFileModel*> modified_models{};

        // TODO: May need to call on FileService for some of these queries. What
        // would respect separation of concerns?
        for (auto& view : views->rFileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;
            if (modified_models.contains(model)) continue;

            modified_models << model;
        }

        if (modified_models.isEmpty()) return true;

        QStringList names{};
        for (auto& model : modified_models) {
            auto meta = model->meta();
            if (!meta) continue;
            names
                << (meta->path().isEmpty() ? meta->title()
                                           : meta->path().toQString());
        }

        auto result = SavePrompt::exec(names, window);
        switch (result.choice) {
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            // TODO: If any of the files need a Save As, we'll want to raise
            // when we prompt there
            // TODO: for (auto& index : result.selectedIndices) { save
            // modified_models[idx] }
            return true;
        case SavePrompt::Discard:
            return true;
        }

        return true;
    }

    virtual bool canCloseAllTabs(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        QList<IFileModel*> modified_models{};

        for (auto& window : windows) {
            for (auto& view : views->rFileViewsIn(window)) {
                if (!view) continue;
                auto model = view->model();
                if (!model || !model->isModified()) continue;
                if (modified_models.contains(model)) continue;

                modified_models << model;
            }
        }

        if (modified_models.isEmpty()) return true;

        QStringList names{};
        for (auto& model : modified_models) {
            auto meta = model->meta();
            if (!meta) continue;
            names
                << (meta->path().isEmpty() ? meta->title()
                                           : meta->path().toQString());
        }

        // Make top window the dialog owner (the list here is reverse z-order)
        auto result = SavePrompt::exec(names, windows.last());
        switch (result.choice) {
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            // TODO: If any of the files need a Save As, we'll want to raise
            // when we prompt there
            // TODO: for (auto& index : result.selectedIndices) { save
            // modified_models[idx] }
            return true;
        case SavePrompt::Discard:
            return true;
        }

        return true;
    }

    // TODO: Factor out common code (likely all this and close all tabs into a
    // check save for all tabs or similar) but only after we've solved the
    // multi-file click and raise problem
    virtual bool canCloseWindow(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        QList<IFileModel*> modified_models{};

        for (auto& view : views->rFileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;
            if (modified_models.contains(model)) continue;

            modified_models << model;
        }

        if (modified_models.isEmpty()) return true;

        QStringList names{};
        for (auto& model : modified_models) {
            auto meta = model->meta();
            if (!meta) continue;
            names
                << (meta->path().isEmpty() ? meta->title()
                                           : meta->path().toQString());
        }

        auto result = SavePrompt::exec(names, window);
        switch (result.choice) {
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            // TODO: for (int idx : result.selectedIndices) { save
            // modified_models[idx] }
            return true;
        case SavePrompt::Discard:
            return true;
        }

        return true;
    }

    // TODO: Factor out common code (likely all this and close all tabs into a
    // check save for all tabs or similar) but only after we've solved the
    // multi-file click and raise problem
    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        QList<IFileModel*> modified_models{};

        for (auto& window : windows) {
            for (auto& view : views->rFileViewsIn(window)) {
                if (!view) continue;
                auto model = view->model();
                if (!model || !model->isModified()) continue;
                if (modified_models.contains(model)) continue;

                modified_models << model;
            }
        }

        if (modified_models.isEmpty()) return true;

        QStringList names{};
        for (auto& model : modified_models) {
            auto meta = model->meta();
            if (!meta) continue;
            names
                << (meta->path().isEmpty() ? meta->title()
                                           : meta->path().toQString());
        }

        auto result = SavePrompt::exec(names, windows.last());
        switch (result.choice) {
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            // TODO: for (int idx : result.selectedIndices) { save
            // modified_models[idx] }
            return true;
        case SavePrompt::Discard:
            return true;
        }

        return true;
    }

private:
    Coco::Path currentBaseDir_ = AppDirs::defaultDocs();
    PathInterceptor pathInterceptor_ = nullptr;
    QFileSystemModel* fsModel_ = new QFileSystemModel(this);
    NotepadMenuModule* menus_ = new NotepadMenuModule(bus, this);

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
                    bus->execute(
                        Commands::OPEN_FILE_AT_PATH,
                        { { "path", qVar(path) } },
                        cmd.context);
                }
            });

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

    void onViewDestroyed_(IFileModel* fileModel)
    {
        if (!fileModel) return;
        if (views->countFor(fileModel) <= 0) files->deleteModel(fileModel);
    }
};

} // namespace Fernanda
