/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Workspace.h"
#include "AboutDialog.h"
#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Application.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "Tr.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

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
        .slot(this, [&] { windows->newWindow(); })
        .shortcut(MenuShortcuts::NEW_WINDOW)

        .separator()

        .action(Tr::nxNewNotebook())
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

        .action(Tr::nxOpenNotebook())
        .slot(
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
        .slot(this, [&, window] { views->closeTab(window, -1); })
        .shortcut(MenuShortcuts::CLOSE_TAB)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseTabEverywhere())
        .slot(this, [&, window] { views->closeTabEverywhere(window, -1); })
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseWindowTabs())
        .slot(this, [&, window] { views->closeWindowTabs(window); })
        .toggle(
            state,
            MenuScope::Window,
            [&, window] { return views->fileViewAt(window, -1); })

        .action(Tr::nxCloseAllTabs())
        .slot(this, [&] { views->closeAllTabs(); })
        .toggle(state, MenuScope::Workspace, [&] { return views->anyViews(); })

        .separator()

        .action(Tr::nxCloseWindow())
        .slot(this, [&, window] { window->close(); })
        .shortcut(MenuShortcuts::CLOSE_WINDOW)

        .action(Tr::nxCloseAllWindows())
        .slot(this, [&] { windows->closeAll(); })

        .separator()

        .action(Tr::nxQuit())
        .slot(app(), &Application::tryQuit, Qt::QueuedConnection)
        .shortcut(MenuShortcuts::QUIT)

        .menu(Tr::nxEditMenu())

        .action(Tr::nxUndo())
        .slot(this, [&, window] { views->undo(window, -1); })
        .shortcut(MenuShortcuts::UNDO)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->hasUndo();
            })

        .action(Tr::nxRedo())
        .slot(this, [&, window] { views->redo(window, -1); })
        .shortcut(MenuShortcuts::REDO)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto model = views->fileModelAt(window, -1);
                return model && model->hasRedo();
            })

        .separator()

        .action(Tr::nxCut())
        .slot(this, [&, window] { views->cut(window, -1); })
        .shortcut(MenuShortcuts::CUT)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::nxCopy())
        .slot(this, [&, window] { views->copy(window, -1); })
        .shortcut(MenuShortcuts::COPY)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .action(Tr::nxPaste())
        .slot(this, [&, window] { views->paste(window, -1); })
        .shortcut(MenuShortcuts::PASTE)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasPaste();
            })

        .action(Tr::nxDelete())
        .slot(this, [&, window] { views->del(window, -1); })
        .shortcut(MenuShortcuts::DEL)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->hasSelection();
            })

        .separator()

        .action(Tr::nxSelectAll())
        .slot(this, [&, window] { views->selectAll(window, -1); })
        .shortcut(MenuShortcuts::SELECT_ALL)
        .toggle(
            state,
            MenuScope::ActiveTab,
            [&, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->supportsEditing();
            })

        .barAction(Tr::nxSettingsMenu())
        // TODO: Settings dialog slot

        .menu(Tr::nxHelpMenu())
        .action(Tr::nxAbout())
        .slot(this, [] { AboutDialog::exec(); })

        .set();
}

} // namespace Fernanda
