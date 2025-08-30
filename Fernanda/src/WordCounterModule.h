#pragma once

#include <QObject>

#include "Coco/Debug.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"

namespace Fernanda {

// Coordinator for Window WordCounters
class WordCounterModule : public IService
{
    Q_OBJECT

public:
    explicit WordCounterModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~WordCounterModule() override { COCO_TRACER; }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
