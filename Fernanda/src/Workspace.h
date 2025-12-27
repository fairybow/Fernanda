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
#include <QKeySequence>
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
#include "AbstractFileView.h"
#include "AppDirs.h"
#include "Bus.h"
#include "ColorBar.h"
#include "ColorBarModule.h"
#include "FileService.h"
#include "Fnx.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "NewNotebookPrompt.h"
#include "Timers.h"
#include "Tr.h"
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
    //SettingsModule* settings =
        //new SettingsModule(AppDirs::userData() / "Settings.ini", bus, this);
    WindowService* windows = new WindowService(bus, this);
    ViewService* views = new ViewService(bus, this);
    FileService* files = new FileService(bus, this);
    TreeViewService* treeViews = new TreeViewService(bus, this);
    ColorBarModule* colorBars = new ColorBarModule(bus, this);

    Coco::Path startDir =
        AppDirs::defaultDocs(); // Since this is currently hardcoded, it goes
                                // here to be shared between Workspace types.
                                // When it's made configurable, it will likely
                                // belong to App

    virtual QAbstractItemModel* treeViewModel() = 0;
    virtual QModelIndex treeViewRootIndex() = 0;

    virtual bool canCloseTab(Window*, int index) { return true; }
    virtual bool canCloseTabEverywhere(Window*, int index) { return true; }
    virtual bool canCloseWindowTabs(Window*) { return true; }
    virtual bool canCloseAllTabs(const QList<Window*>&) { return true; }
    virtual bool canCloseWindow(Window*) { return true; }
    virtual bool canCloseAllWindows(const QList<Window*>&) { return true; }

    virtual void
    workspaceMenuHook(MenuBuilder& builder, MenuState* state, Window* window)
    {
        (void)builder;
        (void)state;
        (void)window;
    }

    virtual void fileMenuOpenActions(MenuBuilder& builder, Window* window) = 0;
    virtual void fileMenuSaveActions(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) = 0;

    enum class MenuScope
    {
        ActiveTab = 0,
        Window,
        Workspace
    };

    void refreshMenus(MenuScope scope)
    {
        for (auto state : menuStates_)
            state->refresh(scope);
    }

    void refreshMenus(Window* window, MenuScope scope)
    {
        if (auto state = menuStates_.value(window)) state->refresh(scope);
    }

private:
    QHash<Window*, QList<QMetaObject::Connection>> activeTabConnections_{};
    QHash<Window*, MenuState*> menuStates_{};

    void setup_()
    {
        //settings->initialize();
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

        connect(
            views,
            &ViewService::activeChanged,
            this,
            &Workspace::onViewsActiveChanged_);

        connect(
            views,
            &ViewService::viewDestroyed,
            this,
            [&](AbstractFileModel* fileModel) {
                (void)fileModel;
                refreshMenus(MenuScope::Window);
                refreshMenus(MenuScope::Workspace);
            });

        windows->setCanCloseHook(this, &Workspace::canCloseWindow);
        windows->setCanCloseAllHook(this, &Workspace::canCloseAllWindows);
        connect(windows, &WindowService::lastWindowClosed, this, [&] {
            emit lastWindowClosed(); // Propagate this signal to App for each
                                     // individual Workspace
        });

        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);

        connectBusEvents_();
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, [&](Window* window) {
            createWindowMenuBar_(window);
        });

        connect(bus, &Bus::windowDestroyed, this, [&](Window* window) {
            delete menuStates_.take(window);

            disconnectOldActiveTab_(window);
            activeTabConnections_.remove(window);

            refreshMenus(MenuScope::Window);
            refreshMenus(MenuScope::Workspace);
        });

        connect(
            bus,
            &Bus::fileModelReadied,
            this,
            [&](Window* window, AbstractFileModel* fileModel) {
                (void)window;
                (void)fileModel;

                refreshMenus(MenuScope::Window);
                refreshMenus(MenuScope::Workspace);
            });

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            [&](AbstractFileModel* fileModel, bool modified) {
                (void)fileModel;
                (void)modified;

                refreshMenus(MenuScope::Window);
                refreshMenus(MenuScope::Workspace);
            });
    }

    void createWindowMenuBar_(Window* window);

    void disconnectOldActiveTab_(Window* window)
    {
        if (!window) return;

        if (auto old = activeTabConnections_.take(window); !old.isEmpty()) {
            for (auto& connection : old)
                disconnect(connection);
        }
    }

private slots:
    void onViewsActiveChanged_(Window* window, AbstractFileView* activeFileView)
    {
        // Both of these even when active view is nullptr!
        disconnectOldActiveTab_(window);
        refreshMenus(window, MenuScope::ActiveTab);

        if (!window || !activeFileView) return;
        auto model = activeFileView->model();
        if (!model) return;

        auto& connections = activeTabConnections_[window];
        auto slot = [&, window] {
            refreshMenus(window, MenuScope::ActiveTab);
        };

        connections << connect(
            model,
            &AbstractFileModel::modificationChanged,
            this,
            slot);

        connections
            << connect(model, &AbstractFileModel::undoAvailable, this, slot);

        connections
            << connect(model, &AbstractFileModel::redoAvailable, this, slot);

        connections << connect(
            activeFileView,
            &AbstractFileView::selectionChanged,
            this,
            slot);

        connections << connect(
            activeFileView,
            &AbstractFileView::clipboardDataChanged,
            this,
            slot);
    }
};

} // namespace Fernanda
