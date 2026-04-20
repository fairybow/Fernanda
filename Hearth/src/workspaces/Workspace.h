/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <utility>

#include <QAbstractItemModel>
#include <QKeySequence>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>

#include <Coco/Bool.h>
#include <Coco/Path.h>

#include "core/AppDirs.h"
#include "core/Files.h"
#include "core/Time.h"
#include "core/Tr.h"
#include "fnx/Fnx.h"
#include "menus/MenuBuilder.h"
#include "menus/MenuShortcuts.h"
#include "menus/MenuState.h"
#include "models/AbstractFileModel.h"
#include "modules/ColorBarModule.h"
#include "modules/StyleModule.h"
#include "modules/WordCounterModule.h"
#include "services/FileService.h"
#include "services/SettingsService.h"
#include "services/TreeViewService.h"
#include "services/ViewService.h"
#include "services/WindowService.h"
#include "ui/ColorBar.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Bus.h"
#include "workspaces/NewNotebookPrompt.h"

namespace Fernanda {

// Base class for Notepad and Notebook workspaces (collection of windows, their
// files, and the filesystems on which they operate). Owns and initializes
// services and modules, and allows path filtering for the Application
class Workspace : public QObject
{
    Q_OBJECT

public:
    virtual bool tryQuit() = 0;

    void beCute() const
    {
        Time::delay(1200, this, [this] { colorBars->pastel(); });
    }

    bool hasWindows() const { return windows->count() > 0; }

    void activate() const
    {
        if (auto active_window = windows->active()) {
            active_window->activate(); // Stack under will raise any others
        }
    }

    // Activates if the Workspace already has windows
    void show()
    {
        if (!hasWindows()) {
            windows->newWindow();
        } else {
            activate();
        }
    }

signals:
    void lastWindowClosed();
    void openNotepadRequested();
    void newNotebookRequested(const Coco::Path& fnxPath);
    void openNotebookRequested(const Coco::Path& fnxPath);

protected:
    struct LocalIniKeys
    {
        QString treeViewDock;
        QString uniqueTabs;
    };

    enum class MenuScope
    {
        ActiveTab = 0,
        Window,
        Workspace
    };

    explicit Workspace(LocalIniKeys localIniKeys, QObject* parent = nullptr)
        : QObject(parent)
        , localIniKeys(std::move(localIniKeys))
    {
        setup_();
    }

    const LocalIniKeys localIniKeys;

    Bus* bus = new Bus(this);
    SettingsService* settings =
        new SettingsService(AppDirs::userData() / "Settings.ini", bus, this);
    WindowService* windows = new WindowService(bus, this);
    ViewService* views = new ViewService(bus, this);
    FileService* files = new FileService(bus, this);
    TreeViewService* treeViews = new TreeViewService(bus, this);
    ColorBarModule* colorBars = new ColorBarModule(bus, this);
    StyleModule* styling = new StyleModule(bus, this);
    WordCounterModule* wordCounters = new WordCounterModule(bus, this);

    // TODO: Local settings maybe
    Coco::Path currentRootDir = AppDirs::defaultDocs();
    Coco::Path rollingOpenStartDir = currentRootDir;

    virtual void autosave() {};

    virtual void newFile(Window*, Files::Type) = 0;
    virtual void importFiles(Window*, const Coco::PathList&) = 0;
    virtual QString importFilter() const = 0;
    virtual QAbstractItemModel* treeViewModel() = 0;
    virtual QModelIndex treeViewRootIndex() = 0;

    // Default `true` for Workspaces with whole-workspace save semantics
    // (Notebook). File-per-document Workspaces (Notepad) override:

    virtual bool canCloseTab(Window*, AbstractFileModel*) { return true; }
    virtual bool canCloseTabEverywhere(Window*, AbstractFileModel*)
    {
        return true;
    }
    virtual bool canCloseSplit(Window*) { return true; }
    virtual bool canCloseWindowTabs(Window*) { return true; }
    virtual bool canCloseAllTabs(const QList<Window*>&) { return true; }
    virtual bool canCloseWindow(Window*) { return true; }
    virtual bool canCloseAllWindows(const QList<Window*>&) { return true; }

    /// TODO NF: Consider only having the New submenu (with Notepad having all
    /// "kinds" and Notebook having same + new folder; although, Notepad could
    /// be allowed to make folders, it would just have to handle it differently)
    virtual void fileMenuOpenActions(MenuBuilder&, Window*) = 0;
    virtual void fileMenuSaveActions(MenuBuilder&, MenuState*, Window*) = 0;

    virtual void
    tabContextMenuSaveActions(MenuBuilder&, Window*, [[maybe_unused]] int index)
    {
    }

    void refreshMenus(MenuScope scope)
    {
        for (auto state : menuStates_) {
            state->refresh(scope);
        }
    }

    void refreshMenus(Window* window, MenuScope scope)
    {
        if (auto state = menuStates_.value(window)) state->refresh(scope);
    }

private:
    Coco::Path rollingOpenFnxStartDir_ = currentRootDir;
    QHash<Window*, QList<QMetaObject::Connection>> activeTabConnections_{};
    QHash<Window*, MenuState*> menuStates_{};
    Time::Ticker* autosaveCue_ = // TODO: Make configurable?
        Time::newTicker(this, &Workspace::autosave, 15000);

    void setup_()
    {
        initializeServices_();

        wireViewService_();
        wireWindowService_();
        wireTreeViewService_();

        connectBusEvents_();
        autosaveCue_->start();
    }

    void initializeServices_()
    {
        for (auto& service :
             std::initializer_list<AbstractService*>{ settings,
                                                      windows,
                                                      views,
                                                      files,
                                                      treeViews,
                                                      colorBars,
                                                      styling,
                                                      wordCounters }) {
            service->initialize();
        }
    }

    void wireViewService_()
    {
        views->setCanCloseTabHook(this, &Workspace::canCloseTab);
        views->setCanCloseTabEverywhereHook(
            this,
            &Workspace::canCloseTabEverywhere);
        views->setCanCloseSplitHook(this, &Workspace::canCloseSplit);
        views->setCanCloseWindowTabsHook(this, &Workspace::canCloseWindowTabs);
        views->setCanCloseAllTabsHook(this, &Workspace::canCloseAllTabs);
        views->setShouldOpenTabHook([this](Window*, AbstractFileModel*) {
            return !settings->get<bool>(localIniKeys.uniqueTabs);
        });

        connect(views, &ViewService::fileViewDestroyed, this, [this] {
            refreshWindowAndWorkspaceMenus_();
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

        connect(
            views,
            &ViewService::tabContextMenuRequested,
            this,
            &Workspace::onTabContextMenuRequested_);

        connect(
            views,
            &ViewService::addButtonContextMenuRequested,
            this,
            &Workspace::onAddButtonContextMenuRequested_);
    }

    void wireWindowService_()
    {
        windows->setCanCloseHook(this, &Workspace::canCloseWindow);
        windows->setCanCloseAllHook(this, &Workspace::canCloseAllWindows);
        connect(windows, &WindowService::lastWindowClosed, this, [this] {
            emit lastWindowClosed(); // Propagate this signal to App for each
                                     // individual Workspace
        });
    }

    void wireTreeViewService_()
    {
        treeViews->setDockWidgetFeatures(
            QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable);
        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);
        treeViews->setVisibilityKey(localIniKeys.treeViewDock);
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, [this](Window* window) {
            window->statusBar(); // Ensure status bar
            createWindowMenuBar_(window);
        });

        connect(bus, &Bus::windowDestroyed, this, [this](Window* window) {
            delete menuStates_.take(window);

            disconnectOldActiveTab_(window);
            activeTabConnections_.remove(window);
            refreshWindowAndWorkspaceMenus_();
        });

        connect(
            bus,
            &Bus::activeFileViewChanged,
            this,
            &Workspace::onBusActiveFileViewChanged_);

        connect(bus, &Bus::splitCountChanged, this, [this](Window* window) {
            refreshMenus(window, MenuScope::Window);
        });

        connect(bus, &Bus::fileModelReadied, this, [this] {
            refreshWindowAndWorkspaceMenus_();
        });

        connect(bus, &Bus::fileModelModificationChanged, this, [this] {
            refreshWindowAndWorkspaceMenus_();
        });
    }

    void createWindowMenuBar_(Window* window);

    void disconnectOldActiveTab_(Window* window)
    {
        if (!window) return;

        if (auto old = activeTabConnections_.take(window); !old.isEmpty()) {
            for (auto& connection : old) {
                disconnect(connection);
            }
        }
    }

    void refreshWindowAndWorkspaceMenus_()
    {
        refreshMenus(MenuScope::Window);
        refreshMenus(MenuScope::Workspace);
    }

private slots:
    // Active view can be nullptr!
    void onBusActiveFileViewChanged_(
        Window* window,
        AbstractFileView* activeFileView)
    {
        // Both of these even when active view is nullptr!
        disconnectOldActiveTab_(window);
        refreshMenus(window, MenuScope::ActiveTab);

        if (!window || !activeFileView) return;
        auto model = activeFileView->model();
        if (!model) return;

        auto& connections = activeTabConnections_[window];
        auto slot = [this, window] {
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

    /// TODO TD: Remove toWindow?
    /// TODO TS
    void
    onTabDragCompleted_(Window* fromWindow, [[maybe_unused]] Window* toWindow)
    {
        refreshWindowAndWorkspaceMenus_();

        // TODO: Assert below?

        // fromWindow may be null (highly unlikely, purely defensive).
        // fileViewsIn checks actual views, not split count, so this is safe
        // even while suppressAutoCollapse_ defers empty-split cleanup
        if (fromWindow && views->fileViewsIn(fromWindow).isEmpty()) {
            fromWindow->close();
        }
    }

    /// TODO TD
    /// TODO TS
    // TODO: Currently, we have the new window's origin right at the drop point.
    // Later, we may want to find a way to instead position the window such that
    // the dropped tab is aligned with the tab's future position in the new
    // window
    void onTabDraggedToNewWindow_(
        Window* sourceWindow,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec)
    {
        if (!tabSpec.isValid()) return;

        auto new_window = windows->newWindow(dropPos);
        if (!new_window) return;

        views->insertTabSpec(new_window, tabSpec);
        refreshWindowAndWorkspaceMenus_();

        // sourceWindow may be null. The tab has already been removed in
        // startDrag_, so the new window must be created regardless.
        // Source close is a cleanup concern that only runs if we know the
        // origin
        if (sourceWindow && views->fileViewsIn(sourceWindow).isEmpty()) {
            sourceWindow->close();
        }
    }

    /// TODO TS
    void onTabContextMenuRequested_(
        Window* window,
        int index,
        const QPoint& globalPos)
    {
        if (!window || globalPos.isNull()) return;

        auto has_multiple_splits = views->splitCount(window) > 1;

        MenuBuilder(MenuBuilder::ContextMenu, window)
            .submenu(Tr::nxDuplicate())
            .action(Tr::nxDuplicateTab())
            .onUserTrigger(
                this,
                [this, window, index] { views->duplicateTab(window, index); })
            .action(Tr::nxDuplicateToSplitLeft())
            .onUserTrigger(
                this,
                [this, window, index] {
                    views->duplicateToSplitLeft(window, index);
                })
            .action(Tr::nxDuplicateToSplitRight())
            .onUserTrigger(
                this,
                [this, window, index] {
                    views->duplicateToSplitRight(window, index);
                })
            .endSubmenu()
            .submenu(Tr::nxSplit())
            .action(Tr::nxSplitLeft())
            .onUserTrigger(
                this,
                [this, window, index] { views->splitLeft(window, index); })
            .action(Tr::nxSplitRight())
            .onUserTrigger(
                this,
                [this, window, index] { views->splitRight(window, index); })
            .endSubmenu()
            .separator()
            .apply([this, window, index](MenuBuilder& b) {
                tabContextMenuSaveActions(b, window, index);
            })
            .separator()
            .action(Tr::nxCloseTab())
            .onUserTrigger(
                this,
                [this, window, index] { views->closeTab(window, index); })
            .action(Tr::nxCloseTabEverywhere())
            .onUserTrigger(
                this,
                [this, window, index] {
                    views->closeTabEverywhere(window, index);
                })
            .action(Tr::nxCloseWindowTabs())
            .onUserTrigger(
                this,
                [this, window] { views->closeWindowTabs(window); })
            .action(Tr::nxCloseAllTabs())
            .onUserTrigger(this, [this] { views->closeAllTabs(); })
            .separatorIf(has_multiple_splits)
            .actionIf(has_multiple_splits, Tr::nxCloseSplit())
            .onUserTrigger(this, [this, window] { views->closeSplit(window); })
            .popup(globalPos);
    }

    void
    onAddButtonContextMenuRequested_(Window* window, const QPoint& globalPos)
    {
        if (!window || globalPos.isNull()) return;

        MenuBuilder(MenuBuilder::ContextMenu, window)
            .apply([this, window](MenuBuilder& b) {
                for (auto type : Files::workspaceCreatableTypes()) {
                    b.action(Files::name(type))
                        .onUserTrigger(this, [this, window, type] {
                            newFile(window, type);
                        });
                }
            })
            .popup(globalPos);
    }
};

} // namespace Fernanda
