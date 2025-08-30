#pragma once

#include <QObject>

#include "Coco/Debug.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"

namespace Fernanda {

//...
class SettingsModule : public IService
{
    Q_OBJECT

public:
    explicit SettingsModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~SettingsModule() override { COCO_TRACER; }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
