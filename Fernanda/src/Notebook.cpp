/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Notebook.h"
#include "AboutDialog.h"
#include "Application.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "Tr.h"
#include "TreeViewService.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

void Notebook::createWindowMenuBar_(Window* window)
{
    if (!window) return;

    // TODO: Figure out which were auto-repeat! (Undo, Redo, Paste, anything
    // else?)
    // TODO: Somehow section-off common code between Notepad and Notebook

    auto state = new MenuState(window, this);
    menuStates_[window] = state;

    MenuBuilder(MenuBuilder::MenuBar, window)
        .menu(Tr::nxFileMenu())

        .action(Tr::nbNewFile())
        .slot(
            this,
            [&, window] { newFile_(window, treeViews->currentIndex(window)); })
        .shortcut(MenuShortcuts::NEW_TAB)

        .action(Tr::nbNewFolder())
        .slot(
            this,
            [&, window] { newVirtualFolder_(treeViews->currentIndex(window)); })

        .action(Tr::Menus::fileNewWindow())
        .slot(this, [&] { windows->newWindow(); })
        .shortcut(MenuShortcuts::NEW_WINDOW)

        .separator()

        .action(Tr::Menus::fileNewNotebook())
        .slot(this, &Notebook::requestNewNotebook)

        .action(Tr::Menus::fileOpenNotebook())
        .slot(this, &Notebook::requestOpenNotebook)

        .separator()

        .action(Tr::nxSave())
        .slot(this, [&, window] { save_(window); })
        .shortcut(MenuShortcuts::SAVE)
        .toggle(
            state,
            menuStateKeys_.GLOBAL,
            [&] { return !fnxPath_.exists() || fnxModel_->isModified(); })

        .action(Tr::nxSaveAs())
        .slot(this, [&, window] { saveAs_(window); })
        .shortcut(MenuShortcuts::SAVE_AS)

        .separator()

        .action(Tr::Menus::fileCloseTab())
        .slot(this, [&, window] { views->closeTab(window, -1); })
        .shortcut(MenuShortcuts::CLOSE_TAB)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::Menus::fileCloseTabEverywhere())
        .slot(this, [&, window] { views->closeTabEverywhere(window, -1); })
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::Menus::fileCloseWindowTabs())
        .slot(this, [&, window] { views->closeWindowTabs(window); })
        .toggle(
            state,
            menuStateKeys_.WINDOW,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::Menus::fileCloseAllTabs())
        .slot(this, [&] { views->closeAllTabs(); })
        .toggle(state, menuStateKeys_.GLOBAL, [&] { return views->anyViews(); })

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

        .menu(Tr::nbMenu())

        .action(Tr::nbOpenNotepad())
        .slot(this, [&] { emit openNotepadRequested(); })

        .action(Tr::nbImportFiles())
        .slot(
            this,
            [&, window] {
                importFiles_(window, treeViews->currentIndex(window));
            })

        .menu(Tr::nxEditMenu())

        .action(Tr::Menus::editUndo())
        .slot(this, [&, window] { views->undo(window, -1); })
        .shortcut(MenuShortcuts::UNDO)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->hasUndo();
            })

        .action(Tr::Menus::editRedo())
        .slot(this, [&, window] { views->redo(window, -1); })
        .shortcut(MenuShortcuts::REDO)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
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
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::Menus::editCopy())
        .slot(this, [&, window] { views->copy(window, -1); })
        .shortcut(MenuShortcuts::COPY)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::Menus::editPaste())
        .slot(this, [&, window] { views->paste(window, -1); })
        .shortcut(MenuShortcuts::PASTE)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasPaste();
            })

        .action(Tr::Menus::editDelete())
        .slot(this, [&, window] { views->del(window, -1); })
        .shortcut(MenuShortcuts::DEL)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .separator()

        .action(Tr::Menus::editSelectAll())
        .slot(this, [&, window] { views->selectAll(window, -1); })
        .shortcut(MenuShortcuts::SELECT_ALL)
        .toggle(
            state,
            menuStateKeys_.ACTIVE_TAB,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->supportsEditing();
            })

        .barAction(Tr::nxSettingsMenu())
        // TODO: Settings dialog slot

        .menu(Tr::nxHelpMenu())
        .action(Tr::Menus::helpAbout())
        .slot(this, [] { AboutDialog::exec(); })

        .set();
}

} // namespace Fernanda
