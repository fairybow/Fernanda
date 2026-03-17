/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>

#include <QAnyStringView>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <Coco/Path.h>

#include "core/Debug.h"

namespace Fernanda {

// Layered settings object that can set an override config. When the override is
// present, the base will not be written to and instead be used to retrieve
// fallback values before resorting to application default. For our purposes,
// this will allow Notebooks to default to Notepad settings where applicable
// before using application default
class TieredSettings : public QObject
{
    Q_OBJECT

public:
    using KeyConverter = std::function<QVariant(const QVariant&)>;

    explicit TieredSettings(
        const Coco::Path& baseConfigPath,
        QObject* parent = nullptr)
        : QObject(parent)
        , baseConfigPath_(baseConfigPath)
    {
        setup_();
    }

    virtual ~TieredSettings() override { TRACER; }

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

        auto stored = maybeConvert_(key, value, &KeyConverters_::toStorage);

        overrideSettings_ ? overrideSettings_->setValue(key, stored)
                          : baseSettings_->setValue(key, stored);
    }

    QVariant value(QAnyStringView key) const
    {
        if (!baseSettings_) return {};

        // Try override, if present. Else, return base value (may be invalid)
        if (overrideSettings_) {
            auto result = overrideSettings_->value(key);
            if (result.isValid())
                return maybeConvert_(key, result, &KeyConverters_::fromStorage);
        }

        auto result = baseSettings_->value(key);
        if (result.isValid())
            return maybeConvert_(key, result, &KeyConverters_::fromStorage);

        return {};
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

    void setKeyConverters(
        const QString& key,
        const KeyConverter& toStorage,
        const KeyConverter& fromStorage)
    {
        keyConverters_[key] = { toStorage, fromStorage };
    }

    void setKeyConverters(
        const QStringList& keys,
        const KeyConverter& toStorage,
        const KeyConverter& fromStorage)
    {
        for (auto& key : keys)
            keyConverters_[key] = { toStorage, fromStorage };
    }

private:
    Coco::Path baseConfigPath_;

    QSettings* baseSettings_ = nullptr;
    QSettings* overrideSettings_ = nullptr;

    struct KeyConverters_
    {
        KeyConverter toStorage;
        KeyConverter fromStorage;
    };

    QHash<QString, KeyConverters_> keyConverters_{};

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

    // `direction` is a pointer-to-member selecting which converter to use
    // (toStorage or fromStorage) from the KeyConverters_ struct. `(*it)`
    // dereferences the hash iterator to get the KeyConverters_ value, then
    // `.*direction` accesses the selected member function
    QVariant maybeConvert_(
        QAnyStringView key,
        const QVariant& value,
        KeyConverter KeyConverters_::* direction) const
    {
        auto it = keyConverters_.find(key.toString());

        if (it != keyConverters_.end() && (*it).*direction)
            return ((*it).*direction)(value);

        return value;
    }
};

} // namespace Fernanda
