#pragma once

#include <QObject>
#include <QSettings>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
// #include "TieredSettings.h"

namespace Fernanda {

/// For now, let's use a simple QSettings and get to Notebook's two-tier
/// settings later
class SettingsModule : public IService
{
    Q_OBJECT

public:
    SettingsModule(
        const Coco::Path& configPath,
        // const Coco::Path& fallbackConfigPath,
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
        , settings_(new QSettings(
              configPath.toQString(),
              QSettings::IniFormat,
              this)) //, settings_(new TieredSettings(configPath,
                     //fallbackConfigPath, this))
    {
        initialize_();
    }

    virtual ~SettingsModule() override { COCO_TRACER; }

private:
    QSettings* settings_;
    // TieredSettings* settings_;

    void initialize_()
    {
        commander->addCommandHandler(Commands::SetSetting, [] {});
        commander->addQueryHandler(Queries::Setting, [] {});
    }
};

} // namespace Fernanda
