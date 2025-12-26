/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QKeyCombination>
#include <QKeySequence>
#include <QObject>
#include <QString>

#include "AboutDialog.h"
#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "MenuState.h"
#include "Tr.h"
#include "ViewService.h"
#include "Window.h"

namespace Fernanda {

namespace MenuStateKeys {

    constexpr auto ACTIVE_TAB = "tab";
    constexpr auto WINDOW = "window";
    constexpr auto GLOBAL = "global";

} // namespace MenuStateKeys

namespace WorkspaceMenu {

    inline void addEdit(
        MenuBuilder& builder,
        MenuState* state,
        QObject* receiver,
        ViewService* views,
        Window* window)
    {
        builder
            .menu(Tr::nxEditMenu())

            .action(Tr::Menus::editUndo())
            .slot(receiver, [views, window] { views->undo(window, -1); })
            .shortcut(MenuShortcuts::UNDO)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->hasUndo();
                })

            .action(Tr::Menus::editRedo())
            .slot(receiver, [views, window] { views->redo(window, -1); })
            .shortcut(MenuShortcuts::REDO)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto model = views->fileModelAt(window, -1);
                    return model && model->hasRedo();
                })

            .separator()

            .action(Tr::Menus::editCut())
            .slot(receiver, [views, window] { views->cut(window, -1); })
            .shortcut(MenuShortcuts::CUT)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .action(Tr::Menus::editCopy())
            .slot(receiver, [views, window] { views->copy(window, -1); })
            .shortcut(MenuShortcuts::COPY)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .action(Tr::Menus::editPaste())
            .slot(receiver, [views, window] { views->paste(window, -1); })
            .shortcut(MenuShortcuts::PASTE)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasPaste();
                })

            .action(Tr::Menus::editDelete())
            .slot(receiver, [views, window] { views->del(window, -1); })
            .shortcut(MenuShortcuts::DEL)
            .toggle(
                state,
                MenuStateKeys::ACTIVE_TAB,
                [views, window] {
                    auto view = views->fileViewAt(window, -1);
                    return view && view->hasSelection();
                })

            .separator()

            .action(Tr::Menus::editSelectAll())
            .slot(receiver, [views, window] { views->selectAll(window, -1); })
            .shortcut(MenuShortcuts::SELECT_ALL)
            .toggle(state, MenuStateKeys::ACTIVE_TAB, [views, window] {
                auto view = views->fileViewAt(window, -1);
                return view && view->supportsEditing();
            });
    }

    inline void addSettings(MenuBuilder& builder)
    {
        builder.barAction(Tr::nxSettingsMenu());
        // TODO: Settings dialog slot
    }

    inline void addHelp(MenuBuilder& builder, QObject* receiver)
    {
        builder.menu(Tr::nxHelpMenu())
            .action(Tr::Menus::helpAbout())
            .slot(receiver, [] { AboutDialog::exec(); });
    }

} // namespace WorkspaceMenu

} // namespace Fernanda
