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

#include "AboutDialog.h"
#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "AppDirs.h"
#include "Bus.h"
#include "ColorBar.h"
#include "ColorBarModule.h"
#include "Constants.h"
#include "FileService.h"
#include "Fnx.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "NewNotebookPrompt.h"
#include "SettingsModule.h"
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
    SettingsModule* settings =
        new SettingsModule(AppDirs::userData() / "Settings.ini", bus, this);
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

    /// TODO TOGGLES (Rest of protected section)

    /// Current plan:
    /// - Move as much as we can into Workspace (like common "active tab" code)
    /// - What refresh triggers belong in Workspace? Do any belong exclusively
    /// in Notepad or Notebook?
    /// - Could start with a createMenu_ method then just call the apply
    /// function (which would direct to Notepad or Notebook menu builder
    /// function)?

    struct MenuStateKeys
    {
        constexpr static auto ACTIVE_TAB = "tab";
        constexpr static auto WINDOW = "window";
        constexpr static auto GLOBAL = "global";
    } menuStateKeys;

    void addNewWindowAction(MenuBuilder& builder)
    {
        builder.action(Tr::Menus::fileNewWindow())
            .slot(this, [&] { windows->newWindow(); })
            .shortcut(MenuShortcuts::NEW_WINDOW);
    }

    void addOpenNotebookActions(MenuBuilder& builder)
    {
        builder.action(Tr::Menus::fileNewNotebook())
            .slot(
                this,
                [&] {
                    // Will allow creation of new Notebook with a prospective
                    // path that is the
                    // same as an existing Notebook's. When saved, the user will
                    // be warned before saving over the existing Notebook!
                    auto name = NewNotebookPrompt::exec();
                    if (name.isEmpty()) return;
                    emit newNotebookRequested(startDir / (name + Fnx::Io::EXT));
                })

            .action(Tr::Menus::fileOpenNotebook())
            .slot(this, [&] {
                // nullptr parent makes the dialog application modal
                auto path = Coco::PathUtil::Dialog::file(
                    nullptr,
                    Tr::nxOpenNotebookCaption(),
                    startDir,
                    Tr::nxOpenNotebookFilter());

                if (path.isEmpty() || !Fnx::Io::isFnxFile(path)) return;
                emit openNotebookRequested(path);
            });
    }

    // TODO: Emit trigger/refresh call for things in workspace IN workspace!
    // e.g., close tab triggers - can probably also split up the keys into more
    // specific sections.
    void
    addCloseTabActions(MenuBuilder& builder, MenuState* state, Window* window)
    {
        builder.action(Tr::Menus::fileCloseTab())
            .slot(this, [&, window] { views->closeTab(window, -1); })
            .shortcut(MenuShortcuts::CLOSE_TAB)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] { return views->fileViewAt(window, -1); })

            .action(Tr::Menus::fileCloseTabEverywhere())
            .slot(this, [&, window] { views->closeTabEverywhere(window, -1); })
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] { return views->fileViewAt(window, -1); })

            .action(Tr::Menus::fileCloseWindowTabs())
            .slot(this, [&, window] { views->closeWindowTabs(window); })
            .toggle(
                state,
                MenuStateKeys::WINDOW,
                [&, window] { return views->fileViewAt(window, -1); })

            .action(Tr::Menus::fileCloseAllTabs())
            .slot(this, [&] { views->closeAllTabs(); })
            .toggle(state, MenuStateKeys::GLOBAL, [&] {
                return views->anyViews();
            });
    }

    void addCloseWindowActions(MenuBuilder& builder, Window* window)
    {
        builder.action(Tr::Menus::fileCloseWindow())
            .slot(this, [&, window] { window->close(); })
            .shortcut(MenuShortcuts::CLOSE_WINDOW)

            .action(Tr::Menus::fileCloseAllWindows())
            .slot(this, [&] { windows->closeAll(); });
    }

    void addQuitAction(MenuBuilder& builder);

    void addEditMenu(MenuBuilder& builder, MenuState* state, Window* window)
    {
        builder
            .menu(Tr::nxEditMenu())

            .action(Tr::Menus::editUndo())
            .slot(this, [&, window] { views->undo(window, -1); })
            .shortcut(MenuShortcuts::UNDO)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->hasUndo();
                })

            .action(Tr::Menus::editRedo())
            .slot(this, [&, window] { views->redo(window, -1); })
            .shortcut(MenuShortcuts::REDO)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->hasRedo();
                })

            .separator()

            .action(Tr::Menus::editCut())
            .slot(this, [&, window] { views->cut(window, -1); })
            .shortcut(MenuShortcuts::CUT)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .action(Tr::Menus::editCopy())
            .slot(this, [&, window] { views->copy(window, -1); })
            .shortcut(MenuShortcuts::COPY)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .action(Tr::Menus::editPaste())
            .slot(this, [&, window] { views->paste(window, -1); })
            .shortcut(MenuShortcuts::PASTE)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasPaste();
                })

            .action(Tr::Menus::editDelete())
            .slot(this, [&, window] { views->del(window, -1); })
            .shortcut(MenuShortcuts::DEL)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [&, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .separator()

            .action(Tr::Menus::editSelectAll())
            .slot(this, [&, window] { views->selectAll(window, -1); })
            .shortcut(MenuShortcuts::SELECT_ALL)
            .toggle(state, MenuStateKeys::ACTIVE_TAB, [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->supportsEditing();
            });
    }

    void addSettingsMenu(MenuBuilder& builder)
    {
        builder.barAction(Tr::nxSettingsMenu());
        // TODO: Settings dialog slot
    }

    void addHelpMenu(MenuBuilder& builder)
    {
        builder.menu(Tr::nxHelpMenu())
            .action(Tr::Menus::helpAbout())
            .slot(this, [] { AboutDialog::exec(); });
    }

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
        connect(windows, &WindowService::lastWindowClosed, this, [&] {
            emit lastWindowClosed(); // Propagate this signal to App for each
                                     // individual Workspace
        });

        treeViews->setModelHook(this, &Workspace::treeViewModel);
        treeViews->setRootIndexHook(this, &Workspace::treeViewRootIndex);
    }
};

} // namespace Fernanda
