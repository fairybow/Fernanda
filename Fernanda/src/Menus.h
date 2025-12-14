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
#include <utility>

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

namespace Shortcuts {

    constexpr auto NEW_TAB = Qt::CTRL | Qt::Key_N;
    constexpr auto NEW_WINDOW = Qt::CTRL | Qt::SHIFT | Qt::Key_N;
    constexpr auto OPEN_FILE = Qt::CTRL | Qt::Key_O;
    constexpr auto SAVE = Qt::CTRL | Qt::Key_S;
    constexpr auto SAVE_AS = Qt::CTRL | Qt::SHIFT | Qt::Key_S;
    constexpr auto SAVE_ALL = Qt::CTRL | Qt::ALT | Qt::Key_S;
    constexpr auto CLOSE_TAB = Qt::CTRL | Qt::Key_W;
    constexpr auto CLOSE_WINDOW = Qt::CTRL | Qt::SHIFT | Qt::Key_W;
    constexpr auto QUIT = Qt::CTRL | Qt::Key_Q;

    constexpr auto UNDO = Qt::CTRL | Qt::Key_Z;
    constexpr auto REDO = Qt::CTRL | Qt::Key_Y;
    constexpr auto CUT = Qt::CTRL | Qt::Key_X;
    constexpr auto COPY = Qt::CTRL | Qt::Key_C;
    constexpr auto PASTE = Qt::CTRL | Qt::Key_V;
    constexpr auto DEL = Qt::Key_Delete;
    constexpr auto SELECT_ALL = Qt::CTRL | Qt::Key_A;

    // TODO: Any remaining key sequences

} // namespace Shortcuts

using Inserter = std::function<void(QMenu*)>;
COCO_BOOL(AutoRepeat);

template <typename SlotT>
inline QAction* makeAction(
    Window* window,
    const QString& text,
    SlotT&& slot,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    if (!window) return nullptr;

    auto action = new QAction(text, window);
    action->connect(
        action,
        &QAction::triggered,
        window,
        std::forward<SlotT>(slot));
    action->setShortcut(keySequence);
    action->setAutoRepeat(autoRepeat);

    return action;
}

inline QAction* makeBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QVariantMap& commandParams,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    if (!bus) return nullptr;

    return makeAction(
        window,
        text,
        [bus, commandId, commandParams, window] {
            bus->execute(commandId, commandParams, window);
        },
        keySequence,
        autoRepeat);
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

// TODO: Go through all handler implementations and see which should use this
// (handlers that don't use a Command parameter)
inline QAction* makeCmdlessBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    if (!bus) return nullptr;

    return makeAction(
        window,
        text,
        [bus, commandId] { bus->execute(commandId); },
        keySequence,
        autoRepeat);
}

namespace Internal {

    // TODO: Deal with moving stuff to source file and/or removing this bespoke
    // method later
    QAction* makeAppQuitAction_(Window* window);

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
            Tr::Menus::fileNewTab(),
            Shortcuts::NEW_TAB); /// *

        common.file.newWindow = makeBusAction(
            bus,
            window,
            Commands::NEW_WINDOW,
            Tr::Menus::fileNewWindow(),
            Shortcuts::NEW_WINDOW); /// *

        common.file.newNotebook = makeCmdlessBusAction(
            bus,
            window,
            Commands::NEW_NOTEBOOK,
            Tr::Menus::fileNewNotebook()); /// *

        common.file.openNotebook = makeCmdlessBusAction(
            bus,
            window,
            Commands::OPEN_NOTEBOOK,
            Tr::Menus::fileOpenNotebook()); /// *

        common.file.closeTab = makeBusAction(
            bus,
            window,
            Commands::CLOSE_TAB,
            Tr::Menus::fileCloseTab(),
            Shortcuts::CLOSE_TAB); /// *

        common.file.closeTabEverywhere = makeBusAction(
            bus,
            window,
            Commands::CLOSE_TAB_EVERYWHERE,
            Tr::Menus::fileCloseTabEverywhere()); /// *

        common.file.closeWindowTabs = makeBusAction(
            bus,
            window,
            Commands::CLOSE_WINDOW_TABS,
            Tr::Menus::fileCloseWindowTabs()); /// *

        common.file.closeAllTabs = makeBusAction(
            bus,
            window,
            Commands::CLOSE_ALL_TABS,
            Tr::Menus::fileCloseAllTabs()); /// *

        common.file.closeWindow = makeAction(
            window,
            Tr::Menus::fileCloseWindow(),
            [window] {
                if (!window) return;
                window->close();
            },
            Shortcuts::CLOSE_WINDOW); /// *

        common.file.closeAllWindows = makeBusAction(
            bus,
            window,
            Commands::CLOSE_ALL_WINDOWS,
            Tr::Menus::fileCloseAllWindows()); /// *

        common.file.quit = makeAppQuitAction_(window); /// *

        common.edit.undo = makeBusAction(
            bus,
            window,
            Commands::UNDO,
            Tr::Menus::editUndo(),
            Shortcuts::UNDO); /// *

        common.edit.redo = makeBusAction(
            bus,
            window,
            Commands::REDO,
            Tr::Menus::editRedo(),
            Shortcuts::REDO); /// *

        common.edit.cut = makeBusAction(
            bus,
            window,
            Commands::CUT,
            Tr::Menus::editCut(),
            Shortcuts::CUT); /// *

        common.edit.copy = makeBusAction(
            bus,
            window,
            Commands::COPY,
            Tr::Menus::editCopy(),
            Shortcuts::COPY); /// *

        common.edit.paste = makeBusAction(
            bus,
            window,
            Commands::PASTE,
            Tr::Menus::editPaste(),
            Shortcuts::PASTE); /// *

        common.edit.del = makeBusAction(
            bus,
            window,
            Commands::DEL,
            Tr::Menus::editDelete(),
            Shortcuts::DEL); /// *

        common.edit.selectAll = makeBusAction(
            bus,
            window,
            Commands::SELECT_ALL,
            Tr::Menus::editSelectAll(),
            Shortcuts::SELECT_ALL); /// *

        common.settings = makeBusAction(
            bus,
            window,
            Commands::SETTINGS_DIALOG,
            Tr::Menus::settings());

        common.help.about = makeCmdlessBusAction(
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
        menu->addAction(common.file.closeTabEverywhere);
        menu->addAction(common.file.closeWindowTabs);
        menu->addAction(common.file.closeAllTabs);
        menu->addSeparator();
        menu->addAction(common.file.closeWindow);
        menu->addAction(common.file.closeAllWindows);
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
