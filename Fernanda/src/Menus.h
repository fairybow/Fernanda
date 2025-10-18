/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAction>
#include <QKeySequence>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Bool.h"

#include "Bus.h"
#include "Commands.h"
#include "MenuActions.h"
#include "Tr.h"
#include "Window.h"

// Utility functions for Notepad and Notebook menu modules
namespace Fernanda::Menus {

COCO_BOOL(AutoRepeat);

inline QAction* makeBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QVariantMap& commandParams,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    if (!window) return nullptr;

    auto action = new QAction(text, window);
    action->connect(action, &QAction::triggered, window, [=] {
        bus->execute(commandId, commandParams, window);
    });
    action->setShortcut(keySequence);
    action->setAutoRepeat(autoRepeat);

    return action;
}

inline QAction* makeBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    return makeBusAction(
        bus,
        window,
        commandId,
        {},
        text,
        keySequence,
        autoRepeat);
}

// TODO: Ensure we pass -1 to certain commands as arg (for "current editor"
// ops)
// TODO: Before we continue, must document commands and summarize menu actions
inline void
initializeCommonActions(Bus* bus, Window* window, CommonMenuActions& common)
{
    if (!bus || !window) return;

    common.file.newTab = makeBusAction(
        bus,
        window,
        Commands::NEW_TAB,
        {},
        Tr::Menus::fileNewTab());

    common.file.newWindow = makeBusAction(
        bus,
        window,
        Commands::NEW_WINDOW,
        {},
        Tr::Menus::fileNewWindow());

    common.file.newNotebook = makeBusAction(
        bus,
        window,
        Commands::NEW_NOTEBOOK,
        {},
        Tr::Menus::fileNewNotebook());

    common.file.openNotebook = makeBusAction(
        bus,
        window,
        Commands::OPEN_NOTEBOOK,
        {},
        Tr::Menus::fileOpenNotebook());

    common.file.closeTab = makeBusAction(
        bus,
        window,
        Commands::CLOSE_TAB,
        {},
        Tr::Menus::fileCloseTab());

    common.file.closeAllTabsInWindow = makeBusAction(
        bus,
        window,
        Commands::CLOSE_ALL_TABS_IN_WINDOW,
        {},
        Tr::Menus::fileCloseAllTabsInWindow());

    common.file.closeWindow = makeBusAction(
        bus,
        window,
        Commands::CLOSE_WINDOW,
        {},
        Tr::Menus::fileCloseWindow());

    common.file.quit =
        makeBusAction(bus, window, Commands::QUIT, {}, Tr::Menus::fileQuit());

    common.edit.undo =
        makeBusAction(bus, window, Commands::UNDO, {}, Tr::Menus::editUndo());

    common.edit.redo =
        makeBusAction(bus, window, Commands::REDO, {}, Tr::Menus::editRedo());

    common.edit.cut =
        makeBusAction(bus, window, Commands::CUT, {}, Tr::Menus::editCut());

    common.edit.copy =
        makeBusAction(bus, window, Commands::COPY, {}, Tr::Menus::editCopy());

    common.edit.paste =
        makeBusAction(bus, window, Commands::PASTE, {}, Tr::Menus::editPaste());

    common.edit.del = makeBusAction(
        bus,
        window,
        Commands::DELETE,
        {},
        Tr::Menus::editDelete());

    common.edit.selectAll = makeBusAction(
        bus,
        window,
        Commands::SELECT_ALL,
        {},
        Tr::Menus::editSelectAll());

    common.settings = makeBusAction(
        bus,
        window,
        Commands::SETTINGS_DIALOG,
        {},
        Tr::Menus::settings());

    common.help.about = makeBusAction(
        bus,
        window,
        Commands::ABOUT_DIALOG,
        {},
        Tr::Menus::helpAbout());
}

} // namespace Fernanda::Menus
