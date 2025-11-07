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
#include <QModelIndex>
#include <QList>
#include <QObject>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AppDirs.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
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
        bus->addCommandHandler(Commands::TREE_VIEW_MODEL, [&] {
            return fsModel_;
        });

        bus->addCommandHandler(Commands::TREE_VIEW_ROOT_INDEX, [&] {
            // Generate the index on-demand from the stored path (don't hold it
            // separately or retrieve via Model::setRootPath)
            if (!fsModel_) return QModelIndex{};
            return fsModel_->index(currentBaseDir_.toQString());
        });

        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            bus->execute(Commands::NEW_TXT_FILE, cmd.context);
        });

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

        bus->addInterceptor(
            Commands::OPEN_FILE_AT_PATH,
            [&](const Command& cmd) {
                if (pathInterceptor_
                    && pathInterceptor_(cmd.param<Coco::Path>("path")))
                    return true;

                return false;
            });

        /// NOT YET
        /*bus->addCommandHandler(PolyCmd::BASE_DIR, [&] {
            return currentBaseDir_.toQString();
        });*/
    }

    void connectBusEvents_()
    {
        connect(
            bus,
            &Bus::treeViewDoubleClicked,
            this,
            &Notepad::onTreeViewDoubleClicked_);
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
};

} // namespace Fernanda
