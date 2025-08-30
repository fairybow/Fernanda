#pragma once

#include <QObject>

#include "Coco/Debug.h"

#include "ColorBar.h"
#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window ColorBars
class ColorBarModule : public IService
{
    Q_OBJECT

public:
    explicit ColorBarModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~ColorBarModule() override { COCO_TRACER; }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
