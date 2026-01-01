/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAnyStringView>
#include <QList>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "Coco/Path.h"

#include "Debug.h"

namespace Fernanda {

// Layered settings object that can set an override config. When the override is
// present, the base will not be written to and instead be used to retrieve
// fallback values before resorting to application default. For our purposes,
// this will allow Notebooks to default to Notepad settings where applicable
// before using application default
class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(
        const Coco::Path& baseConfigPath,
        QObject* parent = nullptr)
        : QObject(parent)
        , baseConfigPath_(baseConfigPath)
    {
        setup_();
    }

    virtual ~Settings() override { TRACER; }

    // setOverride must not be called while groups are open!
    void setOverride(const Coco::Path& configPath)
    {
        if (configPath.isEmpty()) {
            WARN("Override config path empty!");
            return;
        }

        overrideSettings_ = make_(configPath);
    }

    // setOverride must not be called while groups are open!
    void beginGroup(QAnyStringView prefix)
    {
        if (!baseSettings_) return;

        if (overrideSettings_) overrideSettings_->beginGroup(prefix);
        baseSettings_->beginGroup(prefix);
    }

    void endGroup()
    {
        if (!baseSettings_) return;

        if (overrideSettings_) overrideSettings_->endGroup();
        baseSettings_->endGroup();
    }

    bool isWritable() const
    {
        if (!baseSettings_) return false;

        return overrideSettings_ ? overrideSettings_->isWritable()
                                 : baseSettings_->isWritable();
    }

    void setValue(QAnyStringView key, const QVariant& value)
    {
        if (!baseSettings_) return;

        overrideSettings_ ? overrideSettings_->setValue(key, value)
                          : baseSettings_->setValue(key, value);
    }

    QVariant value(QAnyStringView key) const
    {
        if (!baseSettings_) return {};

        // Try override, if present. Else, return base value (may be invalid)
        if (overrideSettings_) {
            auto result = overrideSettings_->value(key);
            if (result.isValid()) return result;
        }

        return baseSettings_->value(key);
    }

    QVariant value(QAnyStringView key, const QVariant& defaultValue) const
    {
        auto result = value(key);
        return result.isValid() ? result : defaultValue;
    }

    template <typename T> T value(QAnyStringView key) const
    {
        return value(key).value<T>();
    }

    template <typename T>
    T value(QAnyStringView key, const QVariant& defaultValue) const
    {
        return value(key, defaultValue).value<T>();
    }

private:
    Coco::Path baseConfigPath_;

    QSettings* baseSettings_ = nullptr;
    QSettings* overrideSettings_ = nullptr;

    void setup_()
    {
        if (baseConfigPath_.isEmpty()) {
            WARN("Base config path empty!");
            return;
        }

        baseSettings_ = make_(baseConfigPath_);
    }

    QSettings* make_(const Coco::Path& path)
    {
        if (path.isEmpty()) return nullptr;
        return new QSettings(path.toQString(), QSettings::IniFormat, this);
    }
};

} // namespace Fernanda
