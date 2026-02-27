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

#include <QAnyStringView>
#include <QHash>
#include <QList>
#include <QString>
#include <QTextOption>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/Utility.h"

#include "AbstractService.h"
#include "Bus.h"
#include "ColorBar.h"
#include "Debug.h"
#include "Ini.h"
#include "SettingsDialog.h"
#include "ThemesPanel.h"
#include "TieredSettings.h"
#include "Timers.h"
#include "Tr.h"

namespace Fernanda {

// Shared settings accessor for the Workspace. Provides Bus-based access to the
// layered Settings object, allowing Services to get/set configuration values
// without owning a Settings instance directly. Usage mirrors direct Settings
// access: `bus->call(GET, {{"key", k}, {"default", d}})` is equivalent to
// `settings->value(key, default)`
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

        auto v = [&](const auto& key,
                     const auto& default_) -> std::pair<QString, QVariant> {
            return { key, settings_->value(key, qVar(default_)) };
        };

        QVariantMap values{
            // Font
            v(Ini::Keys::EDITOR_FONT, Ini::Defaults::font()),

            // Themes (current selection, not the list of available themes)
            v(Ini::Keys::WINDOW_THEME, Ini::Defaults::windowTheme()),
            v(Ini::Keys::EDITOR_THEME, Ini::Defaults::editorTheme()),

            // Key filters
            v(Ini::Keys::KEY_FILTERS_ACTIVE, Ini::Defaults::keyFiltersActive()),
            v(Ini::Keys::KEY_FILTERS_AUTO_CLOSE,
              Ini::Defaults::keyFiltersAutoClose()),
            v(Ini::Keys::KEY_FILTERS_BARGING,
              Ini::Defaults::keyFiltersBarging()),

            // Editor
            v(Ini::Keys::EDITOR_CENTER_ON_SCROLL,
              Ini::Defaults::editorCenterOnScroll()),
            v(Ini::Keys::EDITOR_OVERWRITE, Ini::Defaults::editorOverwrite()),
            v(Ini::Keys::EDITOR_TAB_STOP_DISTANCE,
              Ini::Defaults::editorTabStopDistance()),
            v(Ini::Keys::EDITOR_WRAP_MODE, Ini::Defaults::editorWrapMode()),
            v(Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE,
              Ini::Defaults::editorDoubleClickWhitespace()),
            v(Ini::Keys::EDITOR_LINE_NUMBERS,
              Ini::Defaults::editorLineNumbers()),
            v(Ini::Keys::EDITOR_LINE_HIGHLIGHT,
              Ini::Defaults::editorLineHighlight()),
            v(Ini::Keys::EDITOR_SELECTION_HANDLES,
              Ini::Defaults::editorSelectionHandles()),

            // Word counter
            v(Ini::Keys::WORD_COUNTER_ACTIVE,
              Ini::Defaults::wordCounterActive()),
            v(Ini::Keys::WORD_COUNTER_LINE_COUNT,
              Ini::Defaults::wordCounterLineCount()),
            v(Ini::Keys::WORD_COUNTER_WORD_COUNT,
              Ini::Defaults::wordCounterWordCount()),
            v(Ini::Keys::WORD_COUNTER_CHAR_COUNT,
              Ini::Defaults::wordCounterCharCount()),
            v(Ini::Keys::WORD_COUNTER_SELECTION,
              Ini::Defaults::wordCounterSelection()),
            v(Ini::Keys::WORD_COUNTER_SEL_REPLACE,
              Ini::Defaults::wordCounterSelReplace()),
            v(Ini::Keys::WORD_COUNTER_LINE_POS,
              Ini::Defaults::wordCounterLinePos()),
            v(Ini::Keys::WORD_COUNTER_COL_POS,
              Ini::Defaults::wordCounterColPos()),

            // Color bar
            v(Ini::Keys::COLOR_BAR_ACTIVE, Ini::Defaults::colorBarActive()),
            v(Ini::Keys::COLOR_BAR_POSITION, Ini::Defaults::colorBarPosition())
        };

        dialog_ = new SettingsDialog(
            name_.isEmpty() ? Tr::settingsTitle()
                            : Tr::settingsTitleFormat().arg(name_),
            values,
            window_theme_entries,
            editor_theme_entries);

        connect(
            dialog_,
            &SettingsDialog::settingChanged,
            this,
            [&](const QString& key, const QVariant& value) {
                emit bus->settingChanged(key, value);

                if (debouncers_.contains(key))
                    queueDebouncedSet_(key, value);
                else
                    set(key, value);
            });

        connect(dialog_, &SettingsDialog::finished, this, [&](int result) {
            (void)result;
            delete dialog_;
            dialog_ = nullptr;
        });

        dialog_->open();
    }

    /// TODO TVT
    QVariant get(QAnyStringView key) const { return settings_->value(key); }

    /// TODO TVT
    QVariant get(QAnyStringView key, const QVariant& defaultValue) const
    {
        return settings_->value(key, defaultValue);
    }

    /// TODO TVT
    template <typename T> T get(QAnyStringView key) const
    {
        return settings_->value<T>(key);
    }

    /// TODO TVT
    template <typename T>
    T get(QAnyStringView key, const QVariant& defaultValue) const
    {
        return settings_->value<T>(key, defaultValue);
    }

    void set(const QString& key, const QVariant& value)
    {
        if (!settings_->isWritable()) {
            WARN("Settings not writable; cannot set key: {}", key);
            return;
        }

        settings_->setValue(key, value);
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
            set(cmd.param<QString>("key"), cmd.param("value"));
        });*/
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    TieredSettings* settings_;

    QString name_{};
    SettingsDialog* dialog_ = nullptr;

    QHash<QString, Timers::Debouncer*> debouncers_{};
    QHash<QString, QVariant> pendingValues_{};

    void setup_()
    {
        setupDebouncer_(Ini::Keys::EDITOR_FONT);
        setupDebouncer_(Ini::Keys::WINDOW_THEME);
        setupDebouncer_(Ini::Keys::EDITOR_THEME);
        setupDebouncer_(Ini::Keys::EDITOR_TAB_STOP_DISTANCE);
    }

    void setupDebouncer_(const QString& key)
    {
        debouncers_[key] = new Timers::Debouncer(500, this, [this, key] {
            set(key, pendingValues_.take(key));
        });
    }

    void queueDebouncedSet_(const QString& key, const QVariant& value)
    {
        pendingValues_[key] = value;
        debouncers_[key]->start();
    }
};

} // namespace Fernanda
