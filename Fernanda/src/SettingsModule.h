#pragma once

#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "TieredSettings.h"

namespace Fernanda {

//...
class SettingsModule : public IService
{
    Q_OBJECT

public:
    SettingsModule(
        const Coco::Path& configPath,
        const Coco::Path& fallbackConfigPath,
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
        , settings_(new TieredSettings(configPath, fallbackConfigPath))
    {
        initialize_();
    }

    virtual ~SettingsModule() override { COCO_TRACER; }

private:
    TieredSettings* settings_;

    void initialize_()
    {
        commander->addCommandHandler(Commands::SetSetting, [] {});
        commander->addQueryHandler(Queries::Setting, [] {});
    }
};

} // namespace Fernanda
