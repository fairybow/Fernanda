#pragma once

#include <QObject>
#include <QDialog>
#include <QSettings>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "Utility.h"
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

    QPointer<QDialog> dialog_ = nullptr;

    void initialize_()
    {
        commander->addCommandHandler(Commands::SettingsDialog, [&] {
            openDialog_();
        });

        //commander->addCommandHandler(Commands::SetSetting, [] {});

        commander->addQueryHandler(Queries::Setting, [&](const QVariantMap& params) {
                return settings_->value(
                    to<QString>(params, "key"),
                    params.value("default"));
            });
    }

    void openDialog_()
    {
        if (dialog_) {
            dialog_->raise();
            dialog_->activateWindow();
            return;
        }

        dialog_ = new QDialog(); // Pass settings to this

        connect(dialog_, &QDialog::finished, this, [&](int result) {
            (void)result;
            if (dialog_) dialog_->deleteLater();
        });

        // after subclass, connect to emission of signals changed

        dialog_->open();
    }
};

} // namespace Fernanda
