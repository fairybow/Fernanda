/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
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

using Inserter = std::function<void(QMenu*)>;
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

namespace Internal {

    // TODO: Remove {} for no arg commands (may not need args arg after all)
    // TODO: Add key sequences
    inline void initializeCommonActions_(
        Bus* bus,
        Window* window,
        CommonMenuActions& common)
    {
        if (!bus || !window) return;

        /// * = implemented

        common.file.newTab = makeBusAction(
            bus,
            window,
            Commands::NEW_TAB,
            Tr::Menus::fileNewTab()); /// *

        common.file.newWindow = makeBusAction(
            bus,
            window,
            Commands::NEW_WINDOW,
            Tr::Menus::fileNewWindow()); /// *

        common.file.newNotebook = makeBusAction(
            bus,
            window,
            Commands::NEW_NOTEBOOK,
            Tr::Menus::fileNewNotebook());

        common.file.openNotebook = makeBusAction(
            bus,
            window,
            Commands::OPEN_NOTEBOOK,
            Tr::Menus::fileOpenNotebook());

        common.file.closeTab = makeBusAction(
            bus,
            window,
            Commands::CLOSE_TAB,
            Tr::Menus::fileCloseTab()); /// * (np wip)

        common.file.closeAllTabsInWindow = makeBusAction(
            bus,
            window,
            Commands::CLOSE_ALL_TABS_IN_WINDOW,
            Tr::Menus::fileCloseAllTabsInWindow()); /// * (np wip)

        common.file.closeWindow = makeBusAction(
            bus,
            window,
            Commands::CLOSE_WINDOW,
            Tr::Menus::fileCloseWindow());

        common.file.quit = makeBusAction(
            bus,
            window,
            Commands::QUIT,
            Tr::Menus::fileQuit());

        common.edit.undo = makeBusAction(
            bus,
            window,
            Commands::UNDO,
            Tr::Menus::editUndo()); /// *

        common.edit.redo = makeBusAction(
            bus,
            window,
            Commands::REDO,
            Tr::Menus::editRedo()); /// *

        common.edit.cut = makeBusAction(
            bus,
            window,
            Commands::CUT,
            Tr::Menus::editCut()); /// *

        common.edit.copy = makeBusAction(
            bus,
            window,
            Commands::COPY,
            Tr::Menus::editCopy()); /// *

        common.edit.paste = makeBusAction(
            bus,
            window,
            Commands::PASTE,
            Tr::Menus::editPaste()); /// *

        common.edit.del = makeBusAction(
            bus,
            window,
            Commands::DEL,
            Tr::Menus::editDelete()); /// *

        common.edit.selectAll = makeBusAction(
            bus,
            window,
            Commands::SELECT_ALL,
            Tr::Menus::editSelectAll()); /// *

        common.settings = makeBusAction(
            bus,
            window,
            Commands::SETTINGS_DIALOG,
            Tr::Menus::settings());

        common.help.about = makeBusAction(
            bus,
            window,
            Commands::ABOUT_DIALOG,
            Tr::Menus::helpAbout()); /// *
    }

    inline void addFileMenu_(
        QMenuBar* menuBar,
        CommonMenuActions& common,
        const Inserter& inserter)
    {
        auto menu = new QMenu(Tr::Menus::file(), menuBar);
        menu->addAction(common.file.newTab);
        menu->addAction(common.file.newWindow);
        menu->addSeparator();
        menu->addAction(common.file.newNotebook);
        menu->addAction(common.file.openNotebook);

        // Save section per subclass
        inserter(menu);

        menu->addAction(common.file.closeTab);
        menu->addAction(common.file.closeAllTabsInWindow);
        menu->addSeparator();
        menu->addAction(common.file.closeWindow);
        menu->addSeparator();
        menu->addAction(common.file.quit);
        menuBar->addMenu(menu);
    }

    inline void addEditMenu_(QMenuBar* menuBar, CommonMenuActions& common)
    {
        auto menu = new QMenu(Tr::Menus::edit(), menuBar);
        menu->addAction(common.edit.undo);
        menu->addAction(common.edit.redo);
        menu->addSeparator();
        menu->addAction(common.edit.cut);
        menu->addAction(common.edit.copy);
        menu->addAction(common.edit.paste);
        menu->addAction(common.edit.del);
        menu->addSeparator();
        menu->addAction(common.edit.selectAll);
        menuBar->addMenu(menu);
    }

    // TODO: Implement
    inline void addViewMenu_(QMenuBar* menuBar, CommonMenuActions& common)
    {
        //...
    }

    // Right now, Settings action is added directly to the menu bar
    inline void addSettingsMenu_(QMenuBar* menuBar, CommonMenuActions& common)
    {
        menuBar->addAction(common.settings);
    }

    inline void addHelpMenu_(QMenuBar* menuBar, CommonMenuActions& common)
    {
        auto menu = new QMenu(Tr::Menus::help(), menuBar);
        menu->addAction(common.help.about);
        menuBar->addMenu(menu);
    }

    inline void addMenus_(
        QMenuBar* menuBar,
        CommonMenuActions& common,
        const Inserter& fileMenuInserter)
    {
        Internal::addFileMenu_(menuBar, common, fileMenuInserter);
        Internal::addEditMenu_(menuBar, common);
        Internal::addViewMenu_(menuBar, common);
        Internal::addSettingsMenu_(menuBar, common);
        Internal::addHelpMenu_(menuBar, common);
    }

} // namespace Internal

inline void addNewMenuBar(
    Bus* bus,
    Window* window,
    CommonMenuActions& common,
    const Inserter& fileMenuInserter)
{
    if (!bus || !window) return;
    auto menu_bar = new QMenuBar(window);

    Internal::initializeCommonActions_(bus, window, common);
    Internal::addMenus_(menu_bar, common, fileMenuInserter);
    window->setMenuBar(menu_bar);
}

} // namespace Fernanda::Menus
