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
#include <QList>
#include <QObject>
#include <QString>
#include <QTextOption>
#include <QVariant>

#include "Coco/Path.h"

#include "AbstractService.h"
#include "Bus.h"
#include "ColorBar.h"
#include "Debug.h"
#include "Ini.h"
#include "SettingsDialog.h"
#include "ThemeSelector.h"
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

        QList<ThemeSelector::Entry> window_theme_entries{};

        // Add themeless option using empty path
        window_theme_entries << ThemeSelector::Entry{ Tr::noTheme(), {} };

        // TODO: Don't use pair. Find a sensible location for using a struct
        // with explicit names! Could have all involved (this, SettingsDialog,
        // StyleModule) reuse ThemeSelector::Entry, maybe
        for (auto& theme : bus->call<QList<std::pair<QString, Coco::Path>>>(
                 Bus::WINDOW_THEMES)) {
            window_theme_entries
                << ThemeSelector::Entry{ theme.first,
                                         theme.second }; // name, path
        }

        QList<ThemeSelector::Entry> editor_theme_entries{};
        editor_theme_entries << ThemeSelector::Entry{ Tr::noTheme(), {} };

        for (auto& theme : bus->call<QList<std::pair<QString, Coco::Path>>>(
                 Bus::EDITOR_THEMES)) {
            editor_theme_entries
                << ThemeSelector::Entry{ theme.first,
                                         theme.second }; // name, path
        }

        SettingsDialog::InitialValues initials{
            .font = settings_->value<QFont>(
                Ini::Keys::EDITOR_FONT,
                Ini::Defaults::font()),

            .windowThemes = window_theme_entries,
            .currentWindowTheme = settings_->value<QString>(
                Ini::Keys::WINDOW_THEME,
                Ini::Defaults::windowTheme()),

            .editorThemes = editor_theme_entries,

            // TODO: Any way to get path to work with QSettings?
            .currentEditorTheme = settings_->value<QString>(
                Ini::Keys::EDITOR_THEME,
                Ini::Defaults::editorTheme()),

            /// TODO KFS
            .keyFiltersActive = settings_->value<bool>(
                Ini::Keys::KEY_FILTERS_ACTIVE,
                Ini::Defaults::keyFiltersActive()),
            .keyFiltersAutoClose = settings_->value<bool>(
                Ini::Keys::KEY_FILTERS_AUTO_CLOSE,
                Ini::Defaults::keyFiltersAutoClose()),
            .keyFiltersBarging = settings_->value<bool>(
                Ini::Keys::KEY_FILTERS_BARGING,
                Ini::Defaults::keyFiltersBarging()),

            /// TODO ES
            .editorCenterOnScroll = settings_->value<bool>(
                Ini::Keys::EDITOR_CENTER_ON_SCROLL,
                Ini::Defaults::editorCenterOnScroll()),
            .editorOverwrite = settings_->value<bool>(
                Ini::Keys::EDITOR_OVERWRITE,
                Ini::Defaults::editorOverwrite()),
            .editorTabStopDistance = settings_->value<int>(
                Ini::Keys::EDITOR_TAB_STOP_DISTANCE,
                Ini::Defaults::editorTabStopDistance()),
            .editorWrapMode = settings_->value<QTextOption::WrapMode>(
                Ini::Keys::EDITOR_WRAP_MODE,
                Ini::Defaults::editorWrapMode()),
            .editorDblClickWhitespace = settings_->value<bool>(
                Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE,
                Ini::Defaults::editorDoubleClickWhitespace()),
            .editorLineNumbers = settings_->value<bool>(
                Ini::Keys::EDITOR_LINE_NUMBERS,
                Ini::Defaults::editorLineNumbers()),
            .editorLineHighlight = settings_->value<bool>(
                Ini::Keys::EDITOR_LINE_HIGHLIGHT,
                Ini::Defaults::editorLineHighlight()),
            .editorSelectionHandles = settings_->value<bool>(
                Ini::Keys::EDITOR_SELECTION_HANDLES,
                Ini::Defaults::editorSelectionHandles()),
            .wordCounterActive = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_ACTIVE,
                Ini::Defaults::wordCounterActive()),
            .wordCounterLineCount = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_LINE_COUNT,
                Ini::Defaults::wordCounterLineCount()),
            .wordCounterWordCount = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_WORD_COUNT,
                Ini::Defaults::wordCounterWordCount()),
            .wordCounterCharCount = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_CHAR_COUNT,
                Ini::Defaults::wordCounterCharCount()),
            .wordCounterSelection = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_SELECTION,
                Ini::Defaults::wordCounterSelection()),
            .wordCounterSelReplace = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_SEL_REPLACE,
                Ini::Defaults::wordCounterSelReplace()),
            .wordCounterLinePos = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_LINE_POS,
                Ini::Defaults::wordCounterLinePos()),
            .wordCounterColPos = settings_->value<bool>(
                Ini::Keys::WORD_COUNTER_COL_POS,
                Ini::Defaults::wordCounterColPos()),
            .colorBarActive = settings_->value<bool>(
                Ini::Keys::COLOR_BAR_ACTIVE,
                Ini::Defaults::colorBarActive()),
            .colorBarPosition = settings_->value<ColorBar::Position>(
                Ini::Keys::COLOR_BAR_POSITION,
                Ini::Defaults::colorBarPosition())
        };

        auto title = name_.isEmpty() ? Tr::settingsTitle()
                                     : Tr::settingsTitleFormat().arg(name_);
        dialog_ = new SettingsDialog(title, initials);

        connect(
            dialog_,
            &SettingsDialog::fontChanged,
            this,
            [&](const QFont& font) {
                emit bus->settingChanged(Ini::Keys::EDITOR_FONT, font);
                pendingFont_ = font;
                fontDebouncer_->start();
            });

        connect(
            dialog_,
            &SettingsDialog::windowThemeChanged,
            this,
            [&](const Coco::Path& path) {
                emit bus->settingChanged(Ini::Keys::WINDOW_THEME, qVar(path));
                pendingWindowTheme_ = path;
                windowThemeDebouncer_->start();
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

        /// TODO KFS
        connect(
            dialog_,
            &SettingsDialog::keyFiltersActiveChanged,
            this,
            [&](bool active) {
                emit bus->settingChanged(Ini::Keys::KEY_FILTERS_ACTIVE, active);
                set(Ini::Keys::KEY_FILTERS_ACTIVE, active);
            });

        /// TODO KFS
        connect(
            dialog_,
            &SettingsDialog::keyFiltersAutoCloseChanged,
            this,
            [&](bool autoClose) {
                emit bus->settingChanged(
                    Ini::Keys::KEY_FILTERS_AUTO_CLOSE,
                    autoClose);
                set(Ini::Keys::KEY_FILTERS_AUTO_CLOSE, autoClose);
            });

        /// TODO KFS
        connect(
            dialog_,
            &SettingsDialog::keyFiltersBargingChanged,
            this,
            [&](bool barging) {
                emit bus->settingChanged(
                    Ini::Keys::KEY_FILTERS_BARGING,
                    barging);
                set(Ini::Keys::KEY_FILTERS_BARGING, barging);
            });

        /// TODO ES
        connect(
            dialog_,
            &SettingsDialog::editorCenterOnScrollChanged,
            this,
            [&](bool centerOnScroll) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_CENTER_ON_SCROLL,
                    centerOnScroll);
                set(Ini::Keys::EDITOR_CENTER_ON_SCROLL, centerOnScroll);
            });

        /// TODO ES
        connect(
            dialog_,
            &SettingsDialog::editorOverwriteChanged,
            this,
            [&](bool overwrite) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_OVERWRITE,
                    overwrite);
                set(Ini::Keys::EDITOR_OVERWRITE, overwrite);
            });

        /// TODO ES
        connect(
            dialog_,
            &SettingsDialog::editorTabStopDistanceChanged,
            this,
            [&](int tabStopDistance) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_TAB_STOP_DISTANCE,
                    tabStopDistance);
                pendingEditorTabStopDistance_ = tabStopDistance;
                editorTabStopDistanceDebouncer_->start();
            });

        /// TODO ES
        connect(
            dialog_,
            &SettingsDialog::editorWrapModeChanged,
            this,
            [&](QTextOption::WrapMode wrapMode) {
                emit bus->settingChanged(Ini::Keys::EDITOR_WRAP_MODE, wrapMode);
                set(Ini::Keys::EDITOR_WRAP_MODE, wrapMode);
            });

        connect(
            dialog_,
            &SettingsDialog::editorDblClickWhitespaceChanged,
            this,
            [&](bool dblClickWhitespace) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE,
                    dblClickWhitespace);
                set(Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE, dblClickWhitespace);
            });

        connect(
            dialog_,
            &SettingsDialog::editorLineNumbersChanged,
            this,
            [&](bool lineNumbers) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_LINE_NUMBERS,
                    lineNumbers);
                set(Ini::Keys::EDITOR_LINE_NUMBERS, lineNumbers);
            });

        connect(
            dialog_,
            &SettingsDialog::editorLineHighlightChanged,
            this,
            [&](bool lineHighlight) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_LINE_HIGHLIGHT,
                    lineHighlight);
                set(Ini::Keys::EDITOR_LINE_HIGHLIGHT, lineHighlight);
            });

        connect(
            dialog_,
            &SettingsDialog::editorSelectionHandlesChanged,
            this,
            [&](bool selectionHandles) {
                emit bus->settingChanged(
                    Ini::Keys::EDITOR_SELECTION_HANDLES,
                    selectionHandles);
                set(Ini::Keys::EDITOR_SELECTION_HANDLES, selectionHandles);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterActiveChanged,
            this,
            [&](bool active) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_ACTIVE,
                    active);
                set(Ini::Keys::WORD_COUNTER_ACTIVE, active);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterLineCountChanged,
            this,
            [&](bool lineCount) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_LINE_COUNT,
                    lineCount);
                set(Ini::Keys::WORD_COUNTER_LINE_COUNT, lineCount);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterWordCountChanged,
            this,
            [&](bool wordCount) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_WORD_COUNT,
                    wordCount);
                set(Ini::Keys::WORD_COUNTER_WORD_COUNT, wordCount);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterCharCountChanged,
            this,
            [&](bool charCount) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_CHAR_COUNT,
                    charCount);
                set(Ini::Keys::WORD_COUNTER_CHAR_COUNT, charCount);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterSelectionChanged,
            this,
            [&](bool selection) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_SELECTION,
                    selection);
                set(Ini::Keys::WORD_COUNTER_SELECTION, selection);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterSelReplaceChanged,
            this,
            [&](bool selReplace) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_SEL_REPLACE,
                    selReplace);
                set(Ini::Keys::WORD_COUNTER_SEL_REPLACE, selReplace);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterLinePosChanged,
            this,
            [&](bool linePos) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_LINE_POS,
                    linePos);
                set(Ini::Keys::WORD_COUNTER_LINE_POS, linePos);
            });

        connect(
            dialog_,
            &SettingsDialog::wordCounterColPosChanged,
            this,
            [&](bool colPos) {
                emit bus->settingChanged(
                    Ini::Keys::WORD_COUNTER_COL_POS,
                    colPos);
                set(Ini::Keys::WORD_COUNTER_COL_POS, colPos);
            });

        connect(
            dialog_,
            &SettingsDialog::colorBarActiveChanged,
            this,
            [&](bool active) {
                emit bus->settingChanged(Ini::Keys::COLOR_BAR_ACTIVE, active);
                set(Ini::Keys::COLOR_BAR_ACTIVE, active);
            });

        connect(
            dialog_,
            &SettingsDialog::colorBarPositionChanged,
            this,
            [&](ColorBar::Position position) {
                emit bus->settingChanged(
                    Ini::Keys::COLOR_BAR_POSITION,
                    position);
                set(Ini::Keys::COLOR_BAR_POSITION, position);
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

    static constexpr auto DEBOUNCE_MS_ = 500;

    // TODO: Possible to have one debouncer for all settings?
    Timers::Debouncer* fontDebouncer_ = nullptr;
    QFont pendingFont_{};

    Timers::Debouncer* windowThemeDebouncer_ = nullptr;
    Coco::Path pendingWindowTheme_{};
    Timers::Debouncer* editorThemeDebouncer_ = nullptr;
    Coco::Path pendingEditorTheme_{};

    Timers::Debouncer* editorTabStopDistanceDebouncer_ = nullptr;
    int pendingEditorTabStopDistance_{};

    void setup_()
    {
        fontDebouncer_ = new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
            set(Ini::Keys::EDITOR_FONT, pendingFont_);
        });

        windowThemeDebouncer_ = new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
            set(Ini::Keys::WINDOW_THEME, pendingWindowTheme_.toQString());
        });

        editorThemeDebouncer_ = new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
            set(Ini::Keys::EDITOR_THEME, pendingEditorTheme_.toQString());
        });

        /// TODO ES
        editorTabStopDistanceDebouncer_ =
            new Timers::Debouncer(DEBOUNCE_MS_, this, [&] {
                set(Ini::Keys::EDITOR_TAB_STOP_DISTANCE,
                    pendingEditorTabStopDistance_);
            });
    }
};

} // namespace Fernanda
