/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Workspace.h"

#include <QAction>
#include <QString>

#include "AboutDialog.h"
#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Application.h"
#include "Ini.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "Tr.h"
#include "UpdateDialog.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

// TODO: Race condition risk in menu creation
//
// Menu building assumes all Services have already processed windowCreated
// (e.g., TreeViewService must create the dock before we call
// dockToggleViewAction()).
//
// This works because Workspace::connectBusEvents_() runs AFTER all Service
// initialize() calls in setup_(). Qt delivers signals to slots in connection
// order, so Services respond to windowCreated before Workspace does.
//
// If Service initialization is ever reordered to come after
// connectBusEvents_(), menu creation will fail silently (null actions, missing
// widgets, etc.).
//
// Potential fixes if this becomes a problem:
// - Use Qt::QueuedConnection for Workspace's windowCreated handler
// - Have Services emit "ready" signals after window setup completes
// - Explicitly sequence menu creation after all Services via a dedicated signal
void Workspace::createWindowMenuBar_(Window* window)
{
    if (!window) return;

    // TODO: Figure out which were auto-repeat! (Undo, Redo, Paste, anything
    // else?)

    auto state = new MenuState(window, this);
    menuStates_[window] = state;

    MenuBuilder(MenuBuilder::MenuBar, window)

        .apply([&, state, window](MenuBuilder& builder) {
            workspaceMenuHook(builder, state, window);
        })

        .menu(Tr::nxFileMenu())

        .apply([&, window](MenuBuilder& builder) {
            fileMenuOpenActions(builder, window);
        })

        .action(Tr::nxNewWindow())
        .onUserTrigger(this, [&] { windows->newWindow(); })
        .shortcut(MenuShortcuts::NEW_WINDOW)

        .separator()

        .action(Tr::nxNewNotebook())
        .onUserTrigger(
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

        .action(Tr::nxOpenNotebook())
        .onUserTrigger(
            this,
            [&] {
                // nullptr parent makes the dialog application modal
                auto path = Coco::PathUtil::Dialog::file(
                    nullptr,
                    Tr::nxOpenNotebookCaption(),
                    startDir,
                    Tr::nxOpenNotebookFilter());

                if (path.isEmpty() || !Fnx::Io::isFnxFile(path)) return;
                emit openNotebookRequested(path);
            })

        .separator()

        .apply([&, state, window](MenuBuilder& builder) {
            fileMenuSaveActions(builder, state, window);
        })

        .separator()

        .action(Tr::nxCloseTab())
        .onUserTrigger(this, [&, window] { views->closeTab(window, -1); })
        .shortcut(MenuShortcuts::CLOSE_TAB)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseTabEverywhere())
        .onUserTrigger(
            this,
            [&, window] { views->closeTabEverywhere(window, -1); })
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseWindowTabs())
        .onUserTrigger(this, [&, window] { views->closeWindowTabs(window); })
        .enabledToggle(
            state,
            MenuScope::Window,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseAllTabs())
        .onUserTrigger(this, [&] { views->closeAllTabs(); })
        .enabledToggle(
            state,
            MenuScope::Workspace,
            [&] { return views->anyViews(); })

        .separator()

        .action(Tr::nxCloseWindow())
        .onUserTrigger(this, [&, window] { window->close(); })
        .shortcut(MenuShortcuts::CLOSE_WINDOW)

        .action(Tr::nxCloseAllWindows())
        .onUserTrigger(this, [&] { windows->closeAll(); })

        .separator()

        .action(Tr::nxQuit())
        .onUserTrigger(app(), &Application::tryQuit, Qt::QueuedConnection)
        .shortcut(MenuShortcuts::QUIT)

        .menu(Tr::nxEditMenu())

        .action(Tr::nxUndo())
        .onUserTrigger(this, [&, window] { views->undo(window, -1); })
        .shortcut(MenuShortcuts::UNDO)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->hasUndo();
            })

        .action(Tr::nxRedo())
        .onUserTrigger(this, [&, window] { views->redo(window, -1); })
        .shortcut(MenuShortcuts::REDO)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->hasRedo();
            })

        .separator()

        .action(Tr::nxCut())
        .onUserTrigger(this, [&, window] { views->cut(window, -1); })
        .shortcut(MenuShortcuts::CUT)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::nxCopy())
        .onUserTrigger(this, [&, window] { views->copy(window, -1); })
        .shortcut(MenuShortcuts::COPY)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::nxPaste())
        .onUserTrigger(this, [&, window] { views->paste(window, -1); })
        .shortcut(MenuShortcuts::PASTE)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasPaste();
            })

        .action(Tr::nxDelete())
        .onUserTrigger(this, [&, window] { views->del(window, -1); })
        .shortcut(MenuShortcuts::DEL)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .separator()

        .action(Tr::nxSelectAll())
        .onUserTrigger(this, [&, window] { views->selectAll(window, -1); })
        .shortcut(MenuShortcuts::SELECT_ALL)
        .enabledToggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->supportsEditing();
            })

        /// TODO TVT
        .menu(Tr::nxViewMenu())
        .addAction(treeViews->dockToggleViewAction(window))
        .onToggle(
            this,
            [&, window](bool checked) {
                if (!window || !window->isVisible()) return;
                settings->set(treeViewDockIniKey(), checked);
            })

        .barAction(Tr::nxSettingsMenu())
        .onUserTrigger(this, [&] { settings->openDialog(); })

        .menu(Tr::nxHelpMenu())
        .action(Tr::nxAbout())
        .onUserTrigger(this, [] { AboutDialog::exec(); })
        .action(Tr::nxCheckForUpdates())
        .onUserTrigger(this, [] { UpdateDialog::exec(); })

        .set();
}

} // namespace Fernanda
