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

#include "Bus.h"
#include "Debug.h"
#include "IService.h"

namespace Fernanda {

// Coordinator for Window WordCounters
class WordCounterModule : public IService
{
    Q_OBJECT

public:
    WordCounterModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        initialize_();
    }

    virtual ~WordCounterModule() override { TRACER; }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
