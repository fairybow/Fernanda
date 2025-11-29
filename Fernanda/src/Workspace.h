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
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

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
        timer(1300, this, [&] { colorBars->runAll(ColorBar::Color::Pastel); });
    }

    void newWindow() { windows->newWindow(); }

    void activate() const
    {
        if (auto active_window = windows->active())
            active_window->activate(); // Stack under will raise any others
    }

signals:
    void lastWindowClosed();

protected:
    // TODO: Getters instead?

    Bus* bus = new Bus(this);
    SettingsModule* settings = new SettingsModule(
        AppDirs::userData() / Constants::CONFIG_FILE_NAME,
        bus,
        this);
    WindowService* windows = new WindowService(bus, this);
    ViewService* views = new ViewService(bus, this);
    FileService* files = new FileService(bus, this);
    TreeViewService* treeViews = new TreeViewService(bus, this);
    ColorBarModule* colorBars = new ColorBarModule(bus, this);

    virtual bool canQuit() = 0;

protected:
    virtual QAbstractItemModel* treeViewModel() = 0;
    virtual QModelIndex treeViewRootIndex() = 0;
    virtual void newTab(Window* window) = 0;

    virtual bool canCloseTab(IFileView*) { return true; }
    virtual bool canCloseTabEverywhere(const QList<IFileView*>&)
    {
        return true;
    }
    virtual bool canCloseWindowTabs(const QList<IFileView*>&) { return true; }
    virtual bool canCloseAllTabs(const QList<IFileView*>&) { return true; }
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

        views->setNewTabHook(this, &Workspace::newTab);
        views->setCanCloseTabHook(this, &Workspace::canCloseTab);
        views->setCanCloseTabEverywhereHook(
            this,
            &Workspace::canCloseTabEverywhere);
        views->setCanCloseWindowTabsHook(this, &Workspace::canCloseWindowTabs);
        views->setCanCloseAllTabsHook(this, &Workspace::canCloseAllTabs);

        windows->setCanCloseHook(this, &Workspace::canCloseWindow);
        windows->setCanCloseAllHook(this, &Workspace::canCloseAllWindows);

        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);

        //...

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_();

    void connectBusEvents_()
    {
        // Propagate this Bus signal to App for each individual Workspace
        connect(bus, &Bus::lastWindowClosed, this, [&] {
            emit lastWindowClosed();
        });
    }
};

} // namespace Fernanda
