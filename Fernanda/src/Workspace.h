/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Bus.h"
#include "ColorBar.h"
#include "ColorBarModule.h"
#include "Constants.h"
#include "FileService.h"
#include "SettingsModule.h"
#include "TreeViewModule.h"
#include "Utility.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

COCO_BOOL(NewWindow);

// Base class for Notepad and Notebook workspaces (collection of windows, their
// files, and the filesystems on which they operate). Owns and initializes
// services and modules, and allows path filtering for the Application
class Workspace : public QObject
{
    Q_OBJECT

public:
    Workspace(const Coco::Path& globalConfig, QObject* parent = nullptr)
        : QObject(parent)
        , globalConfig_(globalConfig)
    {
        setup_();
    }

    virtual ~Workspace() override = default;

    // void open(const Session& session)
    // {
    //   // ...open Session...
    //   // emit bus->workspaceOpened();
    // }

    void open(NewWindow withWindow = NewWindow::No)
    {
        // ... Path args?

        if (withWindow) bus->execute(Commands::NEW_WINDOW);
        timer(this, 1300, [&] { bus->execute(Commands::BE_CUTE); });
    }

    void activate() const
    {
        if (auto active_window = bus->call<Window*>(Commands::ACTIVE_WINDOW))
            active_window->activate();
    }

signals:
    void lastWindowClosed();

protected:
    Bus* bus = new Bus(this);
    SettingsModule* settings = nullptr;

private:
    Coco::Path globalConfig_;

    WindowService* windows_ = new WindowService(bus, this);
    ViewService* views_ = new ViewService(bus, this);
    FileService* files_ = new FileService(bus, this);
    TreeViewModule* treeViews_ = new TreeViewModule(bus, this);
    ColorBarModule* colorBars_ = new ColorBarModule(bus, this);

    void setup_()
    {
        settings = new SettingsModule(globalConfig_, bus, this);

        windows_->initialize();
        views_->initialize();
        files_->initialize();
        treeViews_->initialize();
        colorBars_->initialize();
        settings->initialize();

        windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);
        //...

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_();

    void connectBusEvents_()
    {
        /*connect(bus, &Bus::lastWindowClosed, this, [&] {
            emit lastWindowClosed();
        });*/
    }

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        /// return bus->call<bool>(Cmd::CloseWindowViews, window);
        TRACER;
        qDebug() << "Implement";
        return true;
    }
};

} // namespace Fernanda
