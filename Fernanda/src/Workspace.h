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
#include "ColorBarModule.h"
#include "Constants.h"
#include "FileService.h"
#include "SettingsModule.h"
#include "TreeViewModule.h"
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
        initialize_();
    }

    virtual ~Workspace() override = default;

    // void open(const Session& session)
    // {
    //   // ...open Session...
    //   // emit bus->workspaceInitialized();
    // }

    void open(NewWindow withWindow = NewWindow::No)
    {
        if (withWindow) newWindow_();
        emit bus->workspaceInitialized();
    }

    void activate() const { windows_->activateAll(); }

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

    void initialize_()
    {
        settings = new SettingsModule(globalConfig_, bus, this);
        windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);
        //...
        addCommandHandlers_();

        connect(bus, &Bus::lastWindowClosed, this, [&] {
            emit lastWindowClosed();
        });
    }

    void addCommandHandlers_();

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        ///return bus->call<bool>(Cmd::CloseWindowViews, window);
    }

    void newWindow_()
    {
        if (auto window = windows_->make()) window->show();
    }

    virtual QAbstractItemModel* makeTreeViewModel_() = 0;
};

} // namespace Fernanda
