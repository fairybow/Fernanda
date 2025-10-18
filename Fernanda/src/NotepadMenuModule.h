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

    void addMenuBar_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];
        auto menu_bar = new QMenuBar(window);

        // File
        auto file_menu = new QMenu(Tr::Menus::file(), menu_bar);
        file_menu->addAction(actions.common.file.newTab);
        file_menu->addAction(actions.common.file.newWindow);
        file_menu->addSeparator();
        file_menu->addAction(actions.common.file.newNotebook);
        file_menu->addAction(actions.common.file.openNotebook);
        file_menu->addSeparator();

        // Save section per subclass

        file_menu->addSeparator();
        file_menu->addAction(actions.common.file.closeTab);
        file_menu->addAction(actions.common.file.closeAllTabsInWindow);
        file_menu->addSeparator();
        file_menu->addAction(actions.common.file.closeWindow);
        file_menu->addSeparator();
        file_menu->addAction(actions.common.file.quit);
        menu_bar->addMenu(file_menu);

        // Edit
        // TODO: Can be a common function in Menus.h
        auto edit_menu = new QMenu(Tr::Menus::edit(), menu_bar);
        edit_menu->addAction(actions.common.edit.undo);
        edit_menu->addAction(actions.common.edit.redo);
        edit_menu->addSeparator();
        edit_menu->addAction(actions.common.edit.cut);
        edit_menu->addAction(actions.common.edit.copy);
        edit_menu->addAction(actions.common.edit.paste);
        edit_menu->addAction(actions.common.edit.del);
        edit_menu->addSeparator();
        edit_menu->addAction(actions.common.edit.selectAll);
        menu_bar->addMenu(edit_menu);

        // View (TBI)

        // Settings
        // Settings action is added directly to the menu bar
        // TODO: Can be a common function in Menus.h
        menu_bar->addAction(actions.common.settings);

        // Help
        // TODO: Can be a common function in Menus.h
        auto help_menu = new QMenu(Tr::Menus::help(), menu_bar);
        help_menu->addAction(actions.common.help.about);
        menu_bar->addMenu(help_menu);

        window->setMenuBar(menu_bar);
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];

        Menus::initializeCommonActions(bus, window, actions.common);
        // Init notepad actions
        // initactions(actions)
        addMenuBar_(window);
    }
};

} // namespace Fernanda
