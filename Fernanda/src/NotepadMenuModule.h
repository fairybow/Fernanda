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
#include <QObject>

#include "Coco/Debug.h"

#include "Commander.h"
#include "EventBus.h"
#include "MenuModule.h"

namespace Fernanda {

// ...
class NotepadMenuModule : public MenuModule
{
    Q_OBJECT

public:
    NotepadMenuModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : MenuModule(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~NotepadMenuModule() override { COCO_TRACER; }

protected:
    virtual void initializeWorkspaceActions_(Window* window) override
    {
        if (!window) return;
        Actions_ actions{};

        /// WIP
        actions.fileOpen = make(window, "", Tr::Menus::notepadFileOpen());

        /// WIP
        actions.toggles.fileSave = make(
            window,
            Calls::Save,
            Tr::Menus::notepadFileSave(),
            Qt::CTRL | Qt::Key_S);

        /// WIP
        actions.toggles.fileSaveAs = make(
            window,
            Calls::SaveAs,
            Tr::Menus::notepadFileSaveAs(),
            Qt::CTRL | Qt::ALT | Qt::Key_S);

        /// WIP
        actions.toggles.fileSaveAllInWindow = make(
            window,
            Calls::SaveWindow,
            Tr::Menus::notepadFileSaveAllInWindow());

        /// WIP
        actions.toggles.fileSaveAll = make(
            window,
            Calls::SaveAll,
            Tr::Menus::notepadFileSaveAll(),
            Qt::CTRL | Qt::SHIFT | Qt::Key_S);

        actions_[window] = actions;
        // setInitialToggleStates_(window);
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceOpenActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileOpen);
        return true;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceSaveActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.toggles.fileSave);
        fileMenu->addAction(actions.toggles.fileSaveAs);
        fileMenu->addAction(actions.toggles.fileSaveAllInWindow);
        fileMenu->addAction(actions.toggles.fileSaveAll);
        return true;
    }

private:
    struct Actions_
    {
        QAction* fileOpen = nullptr;

        struct Toggles
        {
            QAction* fileSave = nullptr;
            QAction* fileSaveAs = nullptr;
            QAction* fileSaveAllInWindow = nullptr;
            QAction* fileSaveAll = nullptr;
        } toggles;
    };

    QHash<Window*, Actions_> actions_{};

    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
