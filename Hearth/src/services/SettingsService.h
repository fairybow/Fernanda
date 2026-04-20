/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <utility>

#include <QAnyStringView>
#include <QFont>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextOption>
#include <QVariant>

#include <Coco/Path.h>
#include <Coco/Utility.h>

#include "core/Debug.h"
#include "core/Time.h"
#include "core/Tr.h"
#include "services/AbstractService.h"
#include "services/TieredSettings.h"
#include "settings/Ini.h"
#include "settings/SettingsDialog.h"
#include "settings/ThemesPanel.h"
#include "ui/ColorBar.h"
#include "workspaces/Bus.h"

namespace Fernanda {

// Shared settings accessor for the Workspace. Provides Bus-based access to the
// layered Settings object, allowing Services to get/set configuration values
// without owning a Settings instance directly. Usage mirrors direct Settings
// access: `bus->call(GET, {{"key", k}, {"default", d}})` is equivalent to
// `settings->value(key, default)`
//
// Remember, explicit conversion to QVariant is required for anything that
// doesn't have a specialized QVariant constructor (QFont, Coco::Path, etc)
//
// TODO: Make module? IDK
class SettingsService : public AbstractService
{
    Q_OBJECT

public:
    SettingsService(
        const Coco::Path& configPath,
        Bus* bus,
        QObject* parent = nullptr)
        : AbstractService(bus, parent)
        , settings_(new TieredSettings(configPath, this))
    {
        setup_();
    }

    virtual ~SettingsService() override
    {
        TRACER;

        if (dialog_) {
            dialog_->close();
            delete dialog_;
        }
    }

    void setOverrideConfigPath(const Coco::Path& configPath)
    {
        if (!settings_) return;
        settings_->setOverride(configPath);
    }

    void setName(const QString& name) { name_ = name; }

    void openDialog()
    {
        if (dialog_) {
            dialog_->raise();
            dialog_->activateWindow();
            return;
        }

        QList<ThemesPanel::Entry> window_theme_entries{};

        // Add themeless option using empty path
        window_theme_entries << ThemesPanel::Entry{ Tr::noTheme(), {} };

        // TODO: Don't use pair. Find a sensible location for using a struct
        // with explicit names! Could have all involved (this, SettingsDialog,
        // StyleModule) reuse ThemeSelector::Entry, maybe
        for (auto& theme : bus->call<QList<std::pair<QString, Coco::Path>>>(
                 Bus::WINDOW_THEMES)) {
            window_theme_entries
                << ThemesPanel::Entry{ theme.first,
                                       theme.second }; // name, path
        }

        QList<ThemesPanel::Entry> editor_theme_entries{};
        editor_theme_entries << ThemesPanel::Entry{ Tr::noTheme(), {} };

        for (auto& theme : bus->call<QList<std::pair<QString, Coco::Path>>>(
                 Bus::EDITOR_THEMES)) {
            editor_theme_entries
                << ThemesPanel::Entry{ theme.first,
                                       theme.second }; // name, path
        }

        auto& defaults = Ini::defaults();
        Ini::Map current_values{};

        for (auto it = defaults.cbegin(); it != defaults.cend(); ++it) {
            current_values[it.key()] = get(it.key());
        }

        dialog_ = new SettingsDialog(
            name_.isEmpty() ? Tr::settingsTitle()
                            : Tr::settingsTitleFormat().arg(name_),
            current_values,
            window_theme_entries,
            editor_theme_entries);

        connect(
            dialog_,
            &SettingsDialog::settingChanged,
            this,
            [this](const QString& key, const QVariant& value) {
                if (debouncers_.contains(key))
                    queueDebouncedSet_(key, value);
                else
                    set(key, value);
            });

        connect(
            dialog_,
            &SettingsDialog::finished,
            this,
            [this]([[maybe_unused]] int result) {
                delete dialog_;
                dialog_ = nullptr;
            });

        dialog_->open();
    }

    QVariant get(QAnyStringView key) const
    {
        return settings_->value(key, Ini::defaults()[key.toString()]);
    }

    template <typename T> T get(QAnyStringView key) const
    {
        return settings_->value<T>(key, Ini::defaults()[key.toString()]);
    }

    void set(const QString& key, const QVariant& value)
    {
        if (!settings_->isWritable()) {
            WARN("Settings not writable; cannot set key: {}", key);
            return;
        }

        settings_->setValue(key, value);
        INFO("Setting changed: {} = {}", key, value);
        emit bus->settingChanged(key, value);
    }

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Bus::GET_SETTING, [this](const Command& cmd) {
            return get(cmd.param<QString>("key"));
        });

        // TODO: Unused at the moment
        bus->addCommandHandler(Bus::SET_SETTING, [this](const Command& cmd) {
            set(cmd.param<QString>("key"), cmd.param<QVariant>("value"));
        });
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    TieredSettings* settings_;

    QString name_{};
    SettingsDialog* dialog_ = nullptr;

    QHash<QString, Time::Debouncer*> debouncers_{};
    QHash<QString, QVariant> pendingValues_{};

    void setup_()
    {
        setupDebouncer_(Ini::Keys::EDITOR_FONT);
        setupDebouncer_(Ini::Keys::WINDOW_THEME);
        setupDebouncer_(Ini::Keys::EDITOR_THEME);
        setupDebouncer_(Ini::Keys::EDITOR_TAB_STOP_DISTANCE);
        setupDebouncer_(Ini::Keys::EDITOR_WRAP_MODE);
        setupDebouncer_(Ini::Keys::EDITOR_LR_MARGIN);
        setupDebouncer_(Ini::Keys::COLOR_BAR_POSITION);

        /// TODO FT: Right place to register these?
        settings_->setKeyConverters(
            { Ini::Keys::WINDOW_THEME, Ini::Keys::EDITOR_THEME },
            [](const QVariant& v) { return v.value<Coco::Path>().toQString(); },
            [](const QVariant& v) { return qVar(Coco::Path(v.toString())); });

        settings_->setKeyConverters(
            Ini::Keys::EDITOR_FONT,
            [](const QVariant& v) { return v.value<QFont>().toString(); },
            [](const QVariant& v) {
                QFont font{};
                font.fromString(v.toString());
                return qVar(font);
            });
    }

    void setupDebouncer_(const QString& key)
    {
        debouncers_[key] = Time::newDebouncer(
            this,
            [this, key] { set(key, pendingValues_.take(key)); },
            500);
    }

    void queueDebouncedSet_(const QString& key, const QVariant& value)
    {
        pendingValues_[key] = value;
        debouncers_[key]->start();
    }
};

} // namespace Fernanda
