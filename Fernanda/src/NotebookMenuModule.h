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

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
