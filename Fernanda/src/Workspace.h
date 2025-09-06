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

#include "ColorBarModule.h"
#include "Commander.h"
#include "EventBus.h"
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
    Workspace(
        const Coco::Path& configPath,
        const Coco::Path& rootPath,
        QObject* parent = nullptr)
        : QObject(parent)
        , config(configPath)
        , rootPath(rootPath)
    {
        initialize_();
    }

    virtual ~Workspace() override = default;

    // void open(const Session& session)
    // {
    //   // ...open Session...
    //   // emit eventBus->workspaceInitialized();
    // }

    void open(NewWindow withWindow = NewWindow::No)
    {
        if (withWindow) newWindow_();
        emit eventBus->workspaceInitialized();
    }

    void activate() const { windows_->activateAll(); }
    Coco::Path root() const noexcept { return rootPath; }

signals:
    void lastWindowClosed();

protected:
    Coco::Path rootPath;
    Coco::Path config;

    Commander* commander = new Commander(this);
    EventBus* eventBus = new EventBus(this);

    SettingsModule* settings = nullptr;

private:
    WindowService* windows_ = new WindowService(commander, eventBus, this);
    ViewService* views_ = new ViewService(commander, eventBus, this);
    FileService* files_ = new FileService(commander, eventBus, this);
    TreeViewModule* treeViews_ = new TreeViewModule(commander, eventBus, this);
    ColorBarModule* colorBars_ = new ColorBarModule(commander, eventBus, this);

    void initialize_()
    {
        settings = new SettingsModule(config, commander, eventBus, this);
        windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);
        //...
        addCommandHandlers_();

        connect(eventBus, &EventBus::lastWindowClosed, this, [&] {
            emit lastWindowClosed();
        });
    }

    void addCommandHandlers_();

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        return commander->call<bool>(Calls::CloseWindowViews, {}, window);
    }

    void newWindow_()
    {
        if (auto window = windows_->make()) window->show();
    }

    virtual QAbstractItemModel* makeTreeViewModel_() = 0;
};

} // namespace Fernanda
