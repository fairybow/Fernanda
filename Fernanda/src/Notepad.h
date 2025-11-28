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

#include <QFileSystemModel>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSet>

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
#include "TreeViewModule.h"
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

    virtual bool canQuit()
    {
        return windows->count() < 1 || windows->closeAll();
    }

protected:
    virtual bool canCloseTab(IFileView* view)
    {
        auto model = view->model();
        if (!model) return false;

        if (model->isModified() && views->countFor(model) <= 1) {
            views->raise(view);

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

    // TODO: Can perhaps raise first (from end) view in each window?
    virtual bool canCloseTabEverywhere(const QList<IFileView*>& views)
    {
        auto model = views.first()->model();
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
    virtual bool canCloseWindowTabs(const QList<IFileView*>& views)
    {
        // Collect unique modified models that only exist in this window
        QSet<IFileModel*> modified_models{};

        for (auto& view : views) {
            if (!view) continue;
            auto model = view->model();
            if (!model) continue;
            if (!model->isModified()) continue;
            if (this->views->isMultiWindow(model)) continue;

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

    virtual bool canCloseAllTabs(const QList<IFileView*>& views)
    {
        // Collect all unique modified models across all windows
        QSet<IFileModel*> modified_models{};

        for (auto& view : views) {
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

    virtual bool canCloseWindow(Window* window)
    {
        // Collect unique modified models that only exist in this window
        QSet<IFileModel*> modified_models{};

        for (auto& view : views->viewsIn(window)) {
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

    virtual bool canCloseAllWindows(const QList<Window*>& windows)
    {
        // Collect all unique modified models across all windows
        QSet<IFileModel*> modified_models{};

        for (auto& view : views->views()) {
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

        registerPolys_();
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

    void registerPolys_()
    {
        bus->addCommandHandler(Commands::WS_TREE_VIEW_MODEL, [&] {
            return fsModel_;
        });

        bus->addCommandHandler(Commands::WS_TREE_VIEW_ROOT_INDEX, [&] {
            // Generate the index on-demand from the stored path (don't hold it
            // separately or retrieve via Model::setRootPath)
            if (!fsModel_) return QModelIndex{};
            return fsModel_->index(currentBaseDir_.toQString());
        });

        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            files->openOffDiskTxtIn(cmd.context);
        });
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

    void onViewDestroyed_(IFileModel* model)
    {
        if (!model) return;
        if (views->countFor(model) <= 0) files->deleteModel(model);
    }
};

} // namespace Fernanda
