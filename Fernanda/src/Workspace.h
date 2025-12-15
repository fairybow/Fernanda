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
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileModel.h"
#include "AppDirs.h"
#include "Bus.h"
#include "ColorBar.h"
#include "ColorBarModule.h"
#include "Constants.h"
#include "FileService.h"
#include "SettingsModule.h"
#include "Timers.h"
#include "TreeViewService.h"
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
    Workspace(QObject* parent = nullptr)
        : QObject(parent)
    {
        setup_();
    }

    virtual ~Workspace() override = default;

    virtual bool tryQuit() = 0;

    // void open(const Session& session)
    // {
    //   // ...open Session...
    //   // emit bus->workspaceOpened();
    // }

    // TODO: Do we ever need to "open" without opening a window?
    void open(NewWindow withWindow = NewWindow::No)
    {
        // ... Path args?

        if (withWindow) windows->newWindow();

        // TODO: Don't run if no windows...
        timer(1200, this, [&] { colorBars->pastel(); });
    }

    void newWindow() { windows->newWindow(); }

    void activate() const
    {
        if (auto active_window = windows->active())
            active_window->activate(); // Stack under will raise any others
    }

signals:
    void lastWindowClosed();
    void newNotebookRequested(const Coco::Path& fnxPath);
    void openNotebookRequested(const Coco::Path& fnxPath);

protected:
    // TODO: Getters instead?

    Bus* bus = new Bus(this);

    // TODO: If we want this to be explicitly "Notepad.ini" then it shouldn't be
    // in base class. And yet, if we want to use it as the base for each
    // individual Notebook's own settings, it isn't strictly Notepad.ini, then
    // is it?
    SettingsModule* settings =
        new SettingsModule(AppDirs::userData() / "Settings.ini", bus, this);
    WindowService* windows = new WindowService(bus, this);
    ViewService* views = new ViewService(bus, this);
    FileService* files = new FileService(bus, this);
    TreeViewService* treeViews = new TreeViewService(bus, this);
    ColorBarModule* colorBars = new ColorBarModule(bus, this);

    // Since this is currently hardcoded, it goes here to be shared between
    // Workspace types. When it's made configurable, it will likely belong to
    // App
    Coco::Path startDir = AppDirs::defaultDocs();

protected:
    virtual QAbstractItemModel* treeViewModel() = 0;
    virtual QModelIndex treeViewRootIndex() = 0;

    virtual bool canCloseTab(Window*, int index) { return true; }
    virtual bool canCloseTabEverywhere(Window*, int index) { return true; }
    virtual bool canCloseWindowTabs(Window*) { return true; }
    virtual bool canCloseAllTabs(const QList<Window*>&) { return true; }
    virtual bool canCloseWindow(Window*) { return true; }
    virtual bool canCloseAllWindows(const QList<Window*>&) { return true; }

private:
    void setup_()
    {
        settings->initialize();
        windows->initialize();
        views->initialize();
        files->initialize();
        treeViews->initialize();
        colorBars->initialize();

        views->setCanCloseTabHook(this, &Workspace::canCloseTab);
        views->setCanCloseTabEverywhereHook(
            this,
            &Workspace::canCloseTabEverywhere);
        views->setCanCloseWindowTabsHook(this, &Workspace::canCloseWindowTabs);
        views->setCanCloseAllTabsHook(this, &Workspace::canCloseAllTabs);

        windows->setCanCloseHook(this, &Workspace::canCloseWindow);
        windows->setCanCloseAllHook(this, &Workspace::canCloseAllWindows);
        // Propagate this signal to App for each individual Workspace
        connect(windows, &WindowService::lastWindowClosed, this, [&] {
            emit lastWindowClosed();
        });

        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_();

    void connectBusEvents_()
    {
        //...
    }
};

} // namespace Fernanda
