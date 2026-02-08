/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
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
#include <QPoint>
#include <QSize>
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
#include "SettingsService.h"
#include "StyleModule.h"
#include "Timers.h"
#include "Tr.h"
#include "TreeViewService.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

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

    void beCute() const
    {
        Timers::delay(1200, this, [&] { colorBars->pastel(); });
    }

    bool hasWindows() const { return windows->count() > 0; }

    void activate() const
    {
        if (auto active_window = windows->active())
            active_window->activate(); // Stack under will raise any others
    }

    void show()
    {
        if (!hasWindows()) {
            windows->newWindow();
        } else {
            activate();
        }
    }

    virtual bool tryQuit() = 0;

signals:
    void lastWindowClosed();
    void newNotebookRequested(const Coco::Path& fnxPath);
    void openNotebookRequested(const Coco::Path& fnxPath);

protected:
    Bus* bus = new Bus(this);

    SettingsService* settings =
        new SettingsService(AppDirs::userData() / "Settings.ini", bus, this);
    WindowService* windows = new WindowService(bus, this);
    ViewService* views = new ViewService(bus, this);
    FileService* files = new FileService(bus, this);
    TreeViewService* treeViews = new TreeViewService(bus, this);
    ColorBarModule* colorBars = new ColorBarModule(bus, this);
    StyleModule* styling = new StyleModule(bus, this);

    Coco::Path startDir =
        AppDirs::defaultDocs(); // Since this is currently hardcoded, it goes
                                // here to be shared between Workspace types.
                                // When it's made configurable, it will likely
                                // belong to App

    virtual QAbstractItemModel* treeViewModel() = 0;
    virtual QModelIndex treeViewRootIndex() = 0;
    virtual QString treeViewDockIniKey() const = 0; /// TODO TVT

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
        settings->initialize();
        windows->initialize();
        views->initialize();
        files->initialize();
        treeViews->initialize();
        colorBars->initialize();
        styling->initialize();

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
            &ViewService::fileViewDestroyed,
            this,
            [&](AbstractFileView* fileView) {
                (void)fileView;
                refreshMenus(MenuScope::Window);
                refreshMenus(MenuScope::Workspace);
            });

        /// TODO TD
        connect(
            views,
            &ViewService::tabDragCompleted,
            this,
            &Workspace::onTabDragCompleted_);

        /// TODO TD
        connect(
            views,
            &ViewService::tabDraggedToNewWindow,
            this,
            &Workspace::onTabDraggedToNewWindow_);

        windows->setCanCloseHook(this, &Workspace::canCloseWindow);
        windows->setCanCloseAllHook(this, &Workspace::canCloseAllWindows);
        connect(windows, &WindowService::lastWindowClosed, this, [&] {
            emit lastWindowClosed(); // Propagate this signal to App for each
                                     // individual Workspace
        });

        treeViews->setDockWidgetFeatures(
            QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable);
        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);

        connectBusEvents_();
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, [&](Window* window) {
            (void)window->statusBar(); // Ensure status bar
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

        // TODO: Is it fine that basically all file views may emit this signal
        // at once (since it's connected to App::clipboardDataChanged)
        connections << connect(
            activeFileView,
            &AbstractFileView::clipboardDataChanged,
            this,
            slot);
    }

    /// TODO TD
    void onTabDragCompleted_(Window* fromWindow, Window* toWindow)
    {
        refreshMenus(MenuScope::Window);
        refreshMenus(MenuScope::Workspace);

        // Close the source window if it has no remaining tabs
        auto source_tab_widget =
            qobject_cast<TabWidget*>(fromWindow->centralWidget());

        if (source_tab_widget && source_tab_widget->isEmpty())
            fromWindow->close();
    }

    /// TODO TD
    void onTabDraggedToNewWindow_(
        Window* sourceWindow,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec)
    {
        if (!tabSpec.isValid()) return;

        // Approximate the tab's visual offset in the new window to position
        // it so the tab appears roughly where the cursor was released
        constexpr auto tab_top_left_approximate = QPoint(0, 38);

        auto max_tab_size = QSize(225, 34); // Fallback
        if (auto source_tabs =
                qobject_cast<TabWidget*>(sourceWindow->centralWidget()))
            max_tab_size = source_tabs->maximumTabSize();

        auto new_window_top_left =
            dropPos - tab_top_left_approximate
            - tabSpec.relPos(max_tab_size.width(), max_tab_size.height());

        auto new_window = windows->newWindow(new_window_top_left);
        if (!new_window) return;

        views->insertTabSpec(new_window, tabSpec);
        tabSpec.widget->setFocus();

        refreshMenus(MenuScope::Window);
        refreshMenus(MenuScope::Workspace);

        // Close the source window if it has no remaining tabs
        if (sourceWindow) {
            auto source_tab_widget =
                qobject_cast<TabWidget*>(sourceWindow->centralWidget());

            if (source_tab_widget && source_tab_widget->isEmpty())
                sourceWindow->close();
        }
    }
};

} // namespace Fernanda
