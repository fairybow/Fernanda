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

namespace Fernanda {

class Commander;
class EventBus;

// Base class for Workspace's Services and Modules, providing protected
// Commander and EventBus member pointers
class IService : public QObject
{
    Q_OBJECT

public:
    IService(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : QObject(parent)
        , commander(commander)
        , eventBus(eventBus)
    {
    }

    virtual ~IService() = default;

protected:
    Commander* commander;
    EventBus* eventBus;
};

} // namespace Fernanda
