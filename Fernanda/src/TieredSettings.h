#pragma once

#include <QAnyStringView>
#include <QList>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "Coco/Debug.h"
#include "Coco/Path.h"

namespace Fernanda {

// Tiered settings object that takes on optional fallback path on construction.
// When present, the fallback INI will be used to retrieve values if not found
// in the primary INI. The fallback INI is never written to. For our pursposes,
// this will allow Notebooks to default to Notepad settings where applicable
// before using default
class TieredSettings : public QObject
{
    Q_OBJECT

public:
    TieredSettings(
        const Coco::Path& configPath,
        const Coco::Path& fallbackConfigPath,
        QObject* parent = nullptr)
        : QObject(parent)
        , configPath_(configPath)
        , fallbackConfigPath_(fallbackConfigPath)
    {
        initialize_();
    }

    explicit TieredSettings(
        const Coco::Path& configPath,
        QObject* parent = nullptr)
        : TieredSettings(configPath, {}, parent)
    {
    }

    virtual ~TieredSettings() override { COCO_TRACER; }

    void beginGroup(QAnyStringView prefix)
    {
        if (tiers_.isEmpty()) return;

        for (auto& settings_obj : tiers_)
            settings_obj->beginGroup(prefix);
    }

    void endGroup()
    {
        if (tiers_.isEmpty()) return;

        for (auto& settings_obj : tiers_)
            settings_obj->endGroup();
    }

    void setValue(QAnyStringView key, const QVariant& value)
    {
        if (tiers_.isEmpty()) return;
        tiers_[0]->setValue(key, value);
    }

    QVariant value(QAnyStringView key) const
    {
        for (auto& settings_obj : tiers_) {
            auto value = settings_obj->value(key);
            if (value.isValid()) return value;
        }

        return {};
    }

    template <typename T> T value(QAnyStringView key) const
    {
        return value(key).value<T>();
    }

    QVariant value(QAnyStringView key, const QVariant& defaultValue) const
    {
        auto result = value(key);
        return result.isValid() ? result : defaultValue;
    }

    template <typename T>
    T value(QAnyStringView key, const QVariant& defaultValue) const
    {
        return value(key, defaultValue).value<T>();
    }

    bool isWritable() const
    {
        if (tiers_.isEmpty()) return false;
        return tiers_[0]->isWritable();
    }

private:
    Coco::Path configPath_;
    Coco::Path fallbackConfigPath_;
    QList<QSettings*> tiers_{};

    void initialize_()
    {
        if (configPath_.isEmpty()) {
            qWarning() << "Primary config path cannot be empty!";
            return;
        }

        initializeConfig_(configPath_);
        initializeConfig_(fallbackConfigPath_);
    }

    void initializeConfig_(const Coco::Path& path)
    {
        if (path.isEmpty()) return;
        tiers_ << new QSettings(path.toQString(), QSettings::IniFormat, this);
    }
};

} // namespace Fernanda
