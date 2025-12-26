/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Notepad.h"
#include "Application.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "Tr.h"
#include "TreeViewService.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"
#include "WorkspaceMenu.h"

namespace Fernanda {

void Notepad::createWindowMenuBar_(Window* window)
{
    if (!window) return;

    // TODO: Figure out which were auto-repeat! (Undo, Redo, Paste, anything
    // else?)
    // TODO: Somehow section-off common code between Notepad and Notebook

    auto state = new MenuState(window, this);
    menuStates_[window] = state;

    MenuBuilder(MenuBuilder::MenuBar, window)
        .menu(Tr::nxFileMenu())

        .action(Tr::npNewTab())
        .slot(this, [&, window] { newTab_(window); })
        .shortcut(MenuShortcuts::NEW_TAB)

        .action(Tr::npOpenFile())
        .slot(this, [&, window] { openFile_(window); })
        .shortcut(MenuShortcuts::OPEN_FILE)

        .action(Tr::Menus::fileNewWindow())
        .slot(this, [&] { windows->newWindow(); })
        .shortcut(MenuShortcuts::NEW_WINDOW)

        .separator()

        .action(Tr::Menus::fileNewNotebook())
        .slot(this, &Notepad::requestNewNotebook)

        .action(Tr::Menus::fileOpenNotebook())
        .slot(this, &Notepad::requestOpenNotebook)

        .separator()

        .action(Tr::nxSave())
        .slot(this, [&, window] { save_(window); })
        .shortcut(MenuShortcuts::SAVE)
        .toggle(
            state,
            MenuStateKeys::ACTIVE_TAB,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->isModified();
            })

        .action(Tr::nxSaveAs())
        .slot(this, [&, window] { saveAs_(window); })
        .shortcut(MenuShortcuts::SAVE_AS)
        .toggle(
            state,
            MenuStateKeys::ACTIVE_TAB,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->supportsModification();
            })

        .action(Tr::npSaveAllInWindow())
        .slot(this, [&, window] { saveAllInWindow_(window); })
        .toggle(
            state,
            MenuStateKeys::WINDOW,
            [&, window] { return views->anyModifiedFileModelsIn(window); })

        .action(Tr::npSaveAll())
        .slot(this, [&, window] { saveAll_(window); })
        .shortcut(MenuShortcuts::SAVE_ALL)
        .toggle(
            state,
            MenuStateKeys::GLOBAL,
            [&] { return files->anyModified(); })

        .separator()

        .action(Tr::Menus::fileCloseTab())
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
        .toggle(state, MenuStateKeys::GLOBAL, [&] { return views->anyViews(); })

        .separator()

        .action(Tr::Menus::fileCloseWindow())
        .slot(this, [&, window] { window->close(); })
        .shortcut(MenuShortcuts::CLOSE_WINDOW)

        .action(Tr::Menus::fileCloseAllWindows())
        .slot(this, [&] { windows->closeAll(); })

        .separator()

        .action(Tr::Menus::fileQuit())
        .slot(app(), &Application::tryQuit, Qt::QueuedConnection)
        .shortcut(MenuShortcuts::QUIT)

        .apply([&, state, window](MenuBuilder& builder) {
            WorkspaceMenu::addEdit(builder, state, this, views, window);
        })

        .apply(
            [](MenuBuilder& builder) { WorkspaceMenu::addSettings(builder); })

        .apply([&](MenuBuilder& builder) {
            WorkspaceMenu::addHelp(builder, this);
        })

        .set();
}

} // namespace Fernanda
