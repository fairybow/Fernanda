/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>

#include "Coco/Debug.h"

#include "Commander.h"
#include "EventBus.h"
#include "TempNewMenuModule.h"

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
        //...
    }

private:
    struct Actions_
    {
        QAction* fileSaveNotebook = nullptr;
        QAction* fileSaveNotebookAs = nullptr;
        QAction* fileImport = nullptr;
        QAction* fileExport = nullptr;
        QAction* fileOpenNotepad = nullptr;
    };

    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
