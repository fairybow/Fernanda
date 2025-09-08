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

        /// Add commands but reimplement them one at a time

        // File/Open
        actions.fileImportFile =
            make(window, "", Tr::Menus::Notebook::fileImportFile());
        actions.fileOpenNotepad =
            make(window, "", Tr::Menus::Notebook::fileOpenNotepad());

        // File/Save
        actions.toggles.fileSaveArchive =
            make(window, "", Tr::Menus::Notebook::fileSaveArchive());
        actions.fileSaveArchiveAs =
            make(window, "", Tr::Menus::Notebook::fileSaveArchiveAs());
        actions.fileExportFile =
            make(window, "", Tr::Menus::Notebook::fileExportFile());

        actions_[window] = actions;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceFileOpenActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileImportFile);
        fileMenu->addAction(actions.fileOpenNotepad);
        return true;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceFileSaveActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.toggles.fileSaveArchive);
        fileMenu->addAction(actions.fileSaveArchiveAs);
        fileMenu->addAction(actions.fileExportFile);
        return true;
    }

private:
    struct Actions_
    {
        QAction* fileImportFile = nullptr;
        QAction* fileOpenNotepad = nullptr;

        QAction* fileSaveArchiveAs = nullptr;
        QAction* fileExportFile = nullptr;

        struct Toggles
        {
            QAction* fileSaveArchive = nullptr;
        } toggles;
    };

    QHash<Window*, Actions_> actions_{};

    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
