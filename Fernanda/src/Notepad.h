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

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AppDirs.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "IFileModel.h"
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

    PathInterceptor pathInterceptor() const noexcept
    {
        return pathInterceptor_;
    }

    void setPathInterceptor(const PathInterceptor& pathInterceptor)
    {
        pathInterceptor_ = pathInterceptor;
    }

    template <typename ClassT>
    void setPathInterceptor(
        ClassT* object,
        bool (ClassT::*method)(const Coco::Path&))
    {
        pathInterceptor_ = [object, method](const Coco::Path& path) {
            return (object->*method)(path);
        };
    }

    /// TODO CR NEW IMPL WIP =========================================

    /// TODO CR:

    virtual bool canQuit() { return true; }

protected:
    virtual bool canCloseTabHook(IFileView*) { return true; }
    virtual bool canCloseTabEverywhereHook() { return true; }
    virtual bool canCloseWindowTabsHook() { return true; }
    virtual bool canCloseAllTabsHook() { return true; }
    virtual bool canCloseWindowHook() { return true; }
    virtual bool canCloseAllWindowsHook() { return true; }

    /// TODO CR NEW IMPL WIP =========================================

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

        /// TODO CR:
        connect(
            bus,
            &Bus::viewDestroyed,
            this,
            &Notepad::onViewDestroyed_);
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

    /// TODO CR:
    void onViewDestroyed_(IFileModel* model)
    {
        if (!model) return;
        if (views->countFor(model) <= 0) files->deleteModel(model);
    }
};

} // namespace Fernanda

/// TODO CR: Old code:

/*virtual bool canCloseWindow(Window* window) override
{
    if (!window) return false;
    return closeWindowTabs_(window);
}

/// WIP
void registerPolyClosures_()
{
    bus->addCommandHandler(Commands::CLOSE_TAB, [&](const Command& cmd) {
        if (!cmd.context) return false;
        auto index = cmd.param<int>("index", -1);
        auto model = views->modelAt(cmd.context, index);
        if (!model) return false;

        auto is_last_view = views->viewsOn(model) <= 1;

        if (is_last_view) {
            // Check if model is modified
            // If so, prompt save
            // Handle save prompt result
        }

        views->deleteAt(cmd.context, index);
        if (is_last_view) files->deleteModel(model);
        return true;
    });

    bus->addCommandHandler(
        Commands::CLOSE_TAB_EVERYWHERE,
        [&](const Command& cmd) {});

    bus->addCommandHandler(
        Commands::CLOSE_WINDOW_TABS,
        [&](const Command& cmd) { return closeWindowTabs_(cmd.context); });

    bus->addCommandHandler(
        Commands::CLOSE_ALL_TABS,
        [&](const Command& cmd) {});

    // Close window, if we remove close acceptor?

    bus->addCommandHandler(
        Commands::CLOSE_ALL_WINDOWS,
        [&](const Command& cmd) {});

    // Quit? Doesn't really fit, though...
}

/// WIP
bool closeWindowTabs_(Window* window)
{
    if (!window) return false;

    // Get a list of all file models (iterating backward) that are not
    // multi-window and are modified (see 9e6cd80 ViewCloseHelper)

    // Save Prompt (multi-file selection version; Save (with
    // selections, defaulted to all), Discard, or Cancel)

    // Handle prompt result (Cancel return, Discard proceed without
    // saves, Save (any or all selected)

    // If proceeding:
    views->deleteAllIn(window);
    // Delete all deletable models (those that were in that window (modified
    // or not) and not open in other windows)
    return true;
}*/
