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

#include "Coco/Path.h"
#include "Coco/PathUtil.h"
#include "Coco/Utility.h"

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
    { // Generate the index on-demand from the stored path (don't hold it
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
            auto text = meta->path().isEmpty() ? meta->title()
                                               : meta->path().toQString();

            switch (SavePrompt::exec(text, window)) {
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

    // TODO: Can perhaps raise first (from end) view in each window?
    // TODO: Although, could somehow pass window or view or both and just raise
    // for the current window/tab this was used on (it's only called via menu)
    virtual bool
    canCloseTabEverywhere(const QList<IFileView*>& fileViews) override
    {
        auto model = fileViews.first()->model();
        if (!model) return false;

        if (model->isModified()) {
            /*switch (SingleSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save
                return true;
            case Discard:
                return true;
            }*/
        }

        return true;
    }

    // TODO: The multi file save prompt could allow clicking on the path or
    // something to switch to the first view on that file we have available
    // (possibly first from the end)
    virtual bool canCloseWindowTabs(const QList<IFileView*>& fileViews) override
    {
        // Collect unique modified models that only exist in this window
        QSet<IFileModel*> modified_models{};

        // TODO: May need to call on FileService for some of these queries. What
        // would respect separation of concerns?
        for (auto& view : fileViews) {
            if (!view) continue;
            auto model = view->model();
            if (!model) continue;
            if (!model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;

            modified_models << model;
        }

        if (!modified_models.isEmpty()) {
            /*switch (MultiFileSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save (selected)
                return true;
            case Discard:
                return true;
            }*/
        }

        return true;
    }

    virtual bool canCloseAllTabs(const QList<IFileView*>& fileViews) override
    {
        // Collect all unique modified models across all windows
        QSet<IFileModel*> modified_models{};

        for (auto& view : fileViews) {
            if (!view) continue;
            auto model = view->model();
            if (!model) continue;
            if (!model->isModified()) continue;

            modified_models << model;
        }

        if (!modified_models.isEmpty()) {
            /*switch (MultiFileSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save (selected)
                return true;
            case Discard:
                return true;
            }*/
        }

        return true;
    }

    virtual bool canCloseWindow(Window* window) override
    {
        // Collect unique modified models that only exist in this window
        QSet<IFileModel*> modified_models{};

        for (auto& view : views->fileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model) continue;
            if (!model->isModified()) continue;
            if (views->isMultiWindow(model)) continue;

            modified_models << model;
        }

        if (!modified_models.isEmpty()) {
            /*switch (MultiFileSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save (selected)
                return true;
            case Discard:
                return true;
            }*/
        }

        return true;
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        // Collect all unique modified models across all windows
        QSet<IFileModel*> modified_models{};

        for (auto& view : views->fileViews()) {
            if (!view) continue;
            auto model = view->model();
            if (!model) continue;
            if (!model->isModified()) continue;

            modified_models << model;
        }

        if (!modified_models.isEmpty()) {
            /*switch (MultiFileSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save (selected)
                return true;
            case Discard:
                return true;
            }*/
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
