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
class NotebookMenuModule : public MenuModule
{
    Q_OBJECT

public:
    NotebookMenuModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : MenuModule(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~NotebookMenuModule() override { COCO_TRACER; }

protected:
    virtual void initializeWorkspaceActions_(Window* window) override
    {
        if (!window) return;
        Actions_ actions{};

        /// WIP
        actions.fileImport = make(window, "", Tr::Menus::notebookFileImport());

        /// WIP
        actions.fileSave = make(window, "", Tr::Menus::notebookFileSave());

        /// WIP
        actions.fileSaveAs = make(window, "", Tr::Menus::notebookFileSaveAs());

        /// WIP
        actions.fileExport = make(window, "", Tr::Menus::notebookFileExport());

        /// WIP
        actions.fileOpenNotepad =
            make(window, "", Tr::Menus::notebookFileOpenNotepad());

        actions_[window] = actions;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceOpenActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileImport);
        return true;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceSaveActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileSave);
        fileMenu->addAction(actions.fileSaveAs);
        fileMenu->addAction(actions.fileExport);
        return true;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceMiscFileActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileOpenNotepad);
        return true;
    }

private:
    struct Actions_
    {
        QAction* fileImport = nullptr;
        QAction* fileSave = nullptr;
        QAction* fileSaveAs = nullptr;
        QAction* fileExport = nullptr;
        QAction* fileOpenNotepad = nullptr;
    };

    QHash<Window*, Actions_> actions_{};

    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
