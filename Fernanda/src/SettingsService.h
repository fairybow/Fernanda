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
#include <QString>
#include <QVariant>

#include "Coco/Path.h"

#include "AbstractService.h"
#include "Bus.h"
#include "Debug.h"
#include "Settings.h"

namespace Fernanda {

// Shared settings accessor for the Workspace. Provides Bus-based access to the
// layered Settings object, allowing Services to get/set configuration values
// without owning a Settings instance directly. Usage mirrors direct Settings
// access: `bus->call(GET, {{"key", k}, {"default", d}})` is equivalent to
// `settings->value(key, default)`
class SettingsModule : public AbstractService
{
    Q_OBJECT

public:
    SettingsModule(
        const Coco::Path& configPath,
        Bus* bus,
        QObject* parent = nullptr)
        : AbstractService(bus, parent)
        , settings_(settings_ = new Settings(configPath, this))
    {
        setup_();
    }

    virtual ~SettingsModule() override { TRACER; }

    void setOverrideConfigPath(const Coco::Path& configPath)
    {
        if (!settings_) return;
        settings_->setOverride(configPath);
    }

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Bus::GET_SETTING, [&](const Command& cmd) {
            return settings_->value(
                cmd.param<QString>("key"),
                cmd.param("defaultValue"));
        });

        bus->addCommandHandler(Bus::SET_SETTING, [&](const Command& cmd) {
            set_(cmd.param<QString>("key"), cmd.param("value"));
        });
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    Settings* settings_;

    void setup_()
    {
        //...
    }

    void set_(const QString& key, const QVariant& value)
    {
        if (!settings_->isWritable()) {
            WARN("Settings not writable, cannot set key: {}", key);
            return;
        }

        settings_->setValue(key, value);

        // TODO: Notification? When we have consumers that need it
    }
};

} // namespace Fernanda
