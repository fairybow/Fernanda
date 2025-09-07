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
#include <QHash>

#include "Coco/Bool.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"

namespace Fernanda {

// Creates and manages menu bars, actions, and keyboard shortcuts for Windows,
// dynamically updating menu states based on Workspace context and routing
// actions through the Commander
class MenuModule : public IService
{
    Q_OBJECT

public:
    MenuModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~MenuModule() override = default;

protected:
    COCO_BOOL(AutoRepeat);

private:
    struct Actions_
    {
        QAction* fileNewTab = nullptr;
        QAction* fileNewWindow = nullptr;
        QAction* fileNewNotebook = nullptr;
        QAction* fileOpenNotebook = nullptr;
        QAction* fileCloseWindow = nullptr;
        QAction* fileCloseAllWindows = nullptr;
        QAction* fileQuit = nullptr;

        QAction* settingsOpen = nullptr;

        QAction* helpAbout = nullptr;

        struct Toggles
        {
            QAction* fileClose = nullptr;
            QAction* fileCloseAllInWindow = nullptr;
            QAction* fileCloseAll = nullptr;

            QAction* editUndo = nullptr;
            QAction* editRedo = nullptr;
            QAction* editCut = nullptr;
            QAction* editCopy = nullptr;
            QAction* editPaste = nullptr;
            QAction* editDelete = nullptr;
            QAction* editSelectAll = nullptr;

            QAction* viewPreviousTab = nullptr;
            QAction* viewNextTab = nullptr;
            QAction* viewPreviousWindow = nullptr;
            QAction* viewNextWindow = nullptr;
        } toggles;
    };

    QHash<Window*, Actions_> windowActions_{};

    void initialize_()
    {
        connect(
            eventBus,
            &EventBus::windowCreated,
            this,
            &MenuModule::onWindowCreated_);

        //...
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        //initializeActions_(window);
        //setupMenuBar_(window);
    }
};

} // namespace Fernanda
