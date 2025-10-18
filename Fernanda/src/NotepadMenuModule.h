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
#include <QHash>
#include <QMenu>
#include <QMenuBar>
#include <QObject>

#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "IService.h"
#include "MenuActions.h"
#include "Menus.h"
#include "Tr.h"

namespace Fernanda {

// ...
class NotepadMenuModule : public IService
{
    Q_OBJECT

public:
    NotepadMenuModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        // setup_();
    }

    virtual ~NotepadMenuModule() override { TRACER; }

protected:
    virtual void registerBusCommands() override {}

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &NotepadMenuModule::onWindowCreated_);

        connect(bus, &Bus::windowDestroyed, this, [&](Window* window) {
            // TODO: Some or all of this could also be common util functions in
            // Menus.h
            actions_.remove(window);

            /*if (auto cx = activeTabConnections_.take(window);
                !cx.isEmpty()) {
                for (auto& connection : cx)
                    disconnect(connection);
            }*/
        });
    }

private:
    QHash<Window*, NotepadMenuActions> actions_{};

    // TODO: Remove {} for no arg commands
    // TODO: Add key sequences
    void initializeActions_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];

        actions.file.openFile = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEPAD_OPEN_FILE,
            {},
            Tr::Menus::fileNotepadOpen());

        actions.file.save = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEPAD_SAVE,
            {},
            Tr::Menus::fileNotepadSave());

        actions.file.saveAs = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEPAD_SAVE_AS,
            {},
            Tr::Menus::fileNotepadSaveAs());

        actions.file.saveAllInWindow = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEPAD_SAVE_ALL_IN_WINDOW,
            {},
            Tr::Menus::fileNotepadSaveAllInWindow());

        actions.file.saveAll = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEPAD_SAVE_ALL,
            {},
            Tr::Menus::fileNotepadSaveAll());
    }

    // TODO: Make whatever can be a Menus util into one
    void addMenuBar_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];
        auto menu_bar = new QMenuBar(window);

        Menus::addFileMenu(menu_bar, actions.common, [&](QMenu* menu) {
            menu->addSeparator();
            menu->addAction(actions.file.openFile);
            menu->addSeparator();
            menu->addAction(actions.file.save);
            menu->addAction(actions.file.saveAs);
            menu->addAction(actions.file.saveAllInWindow);
            menu->addAction(actions.file.saveAll);
            menu->addSeparator();
        });

        Menus::addCommonMenus(menu_bar, actions.common);
        window->setMenuBar(menu_bar);
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];

        Menus::initializeCommonActions(bus, window, actions.common);
        initializeActions_(window);
        addMenuBar_(window);

        // In future, can be:
        // init notepad actions: initializeActions_
        // menus.h does everything else: Menus::make(bus, window, actions, [&]
        // {}) with file menu inserter
    }
};

} // namespace Fernanda
