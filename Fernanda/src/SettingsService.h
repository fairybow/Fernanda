/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <utility>

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include "Coco/Path.h"

#include "AbstractService.h"
#include "Bus.h"
#include "Debug.h"
#include "Ini.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "ThemeSelector.h"
#include "Timers.h"
#include "Tr.h"

namespace Fernanda {

// Shared settings accessor for the Workspace. Provides Bus-based access to the
// layered Settings object, allowing Services to get/set configuration values
// without owning a Settings instance directly. Usage mirrors direct Settings
// access: `bus->call(GET, {{"key", k}, {"default", d}})` is equivalent to
// `settings->value(key, default)`
class SettingsService : public AbstractService
{
    Q_OBJECT

public:
    SettingsService(
        const Coco::Path& configPath,
        Bus* bus,
        QObject* parent = nullptr)
        : AbstractService(bus, parent)
        , settings_(new Settings(configPath, this))
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

        QList<ThemeSelector::Entry> editor_theme_entries{};

        // Add themeless option using empty path
        editor_theme_entries << ThemeSelector::Entry{ Tr::noTheme(), {} };

        // TODO: Don't use pair. Find a sensible location for using a struct
        // with explicit names! Could have all involved (this, SettingsDialog,
        // StyleModule) reuse ThemeSelector::Entry, maybe
        for (auto& theme : bus->call<QList<std::pair<QString, Coco::Path>>>(
                 Bus::EDITOR_THEMES)) {
            editor_theme_entries
                << ThemeSelector::Entry{ theme.first,
                                         theme.second }; // name, path
        }

        SettingsDialog::InitialValues initials{
            .font =
                settings_->value<QFont>(Ini::Keys::FONT, Ini::Defaults::font()),
            .fontSizeMin = Ini::Defaults::FONT_SIZE_MIN,
            .fontSizeMax = Ini::Defaults::FONT_SIZE_MAX,

            .editorThemes = editor_theme_entries,

            // TODO: Any way to get path to work with QSettings?
            .currentEditorTheme = settings_->value<QString>(
                Ini::Keys::EDITOR_THEME,
                Ini::Defaults::editorTheme()),
            // TODO: Window themes
        };

        auto title = name_.isEmpty() ? Tr::settingsTitle()
                                     : Tr::settingsTitleFormat().arg(name_);
        dialog_ = new SettingsDialog(title, initials);

        connect(
            dialog_,
            &SettingsDialog::fontChanged,
            this,
            [&](const QFont& font) {
                emit bus->settingChanged(Ini::Keys::FONT, font);
                pendingFont_ = font;
                fontDebouncer_->start();
            });

        connect(
            dialog_,
            &SettingsDialog::editorThemeChanged,
            this,
            [&](const Coco::Path& path) {
                emit bus->settingChanged(Ini::Keys::EDITOR_THEME, qVar(path));
                pendingEditorTheme_ = path;
                editorThemeDebouncer_->start();
            });

        connect(dialog_, &SettingsDialog::finished, this, [&](int result) {
            (void)result;
            delete dialog_;
            dialog_ = nullptr;
        });

        dialog_->open();
    }

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Bus::GET_SETTING, [&](const Command& cmd) {
            return settings_->value(
                cmd.param<QString>("key"),
                cmd.param("defaultValue"));
        });

        /*bus->addCommandHandler(Bus::SET_SETTING, [&](const Command& cmd) {
            set_(cmd.param<QString>("key"), cmd.param("value"));
        });*/
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    Settings* settings_;

    QString name_{};
    SettingsDialog* dialog_ = nullptr;

    static constexpr auto DEBOUNCE_MS_ = 500;

    Timers::Debouncer* fontDebouncer_ =
        nullptr; // TODO: Possible to have one debouncer for all settings?
    QFont pendingFont_{};
    Timers::Debouncer* editorThemeDebouncer_ =
        nullptr; // TODO: Possible to have one debouncer for all settings?
    Coco::Path pendingEditorTheme_{};

    void setup_()
    {
        fontDebouncer_ = new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
            set_(Ini::Keys::FONT, pendingFont_);
        });

        editorThemeDebouncer_ = new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
            set_(Ini::Keys::EDITOR_THEME, pendingEditorTheme_.toQString());
        });
    }

    void set_(const QString& key, const QVariant& value)
    {
        if (!settings_->isWritable()) {
            WARN("Settings not writable; cannot set key: {}", key);
            return;
        }

        settings_->setValue(key, value);
    }
};

} // namespace Fernanda
