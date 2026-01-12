/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <algorithm>
#include <utility>

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileView.h"
#include "AbstractService.h"
#include "Bus.h"
#include "Debug.h"
#include "Ini.h"
#include "TextFileView.h"
#include "Themes.h"
#include "Window.h"

namespace Fernanda {

// TODO: Install watcher on theme paths for hot reload
class StyleModule : public AbstractService
{
    Q_OBJECT

public:
    StyleModule(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~StyleModule() override { TRACER; }

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Bus::WINDOW_THEMES, [&] {
            return themeData_(windowThemes_);
        });

        bus->addCommandHandler(Bus::EDITOR_THEMES, [&] {
            return themeData_(editorThemes_);
        });
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::settingChanged,
            this,
            &StyleModule::onBusSettingChanged_);

        connect(bus, &Bus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            windows_ << window;

            // TODO: Search use of Window::destroyed and replace with this (also
            // remove the note where I said only use Window::destroyed lol)
            connect(window, &QObject::destroyed, this, [&, window] {
                windows_.remove(window);
            });

            // Lazy-load current theme path if not yet loaded
            if (!initialWindowThemeLoaded_) {
                currentWindowThemePath_ = bus->call<QString>(
                    Bus::GET_SETTING,
                    { { "key", Ini::Keys::WINDOW_THEME },
                      { "defaultValue", Ini::Defaults::windowTheme() } });

                initialWindowThemeLoaded_ = true;
            }

            applyWindowTheme_(
                window,
                findWindowTheme_(currentWindowThemePath_));
        });

        connect(
            bus,
            &Bus::fileViewCreated,
            this,
            [&](AbstractFileView* fileView) {
                auto text_view = qobject_cast<TextFileView*>(fileView);
                if (!text_view) return;

                textFileViews_ << text_view;

                connect(text_view, &QObject::destroyed, this, [&, text_view] {
                    textFileViews_.remove(text_view);
                });

                // Lazy-load current theme path if not yet loaded
                if (!initialEditorThemeLoaded_) {
                    currentEditorThemePath_ = bus->call<QString>(
                        Bus::GET_SETTING,
                        { { "key", Ini::Keys::EDITOR_THEME },
                          { "defaultValue", Ini::Defaults::editorTheme() } });

                    initialEditorThemeLoaded_ = true;
                }

                applyEditorTheme_(
                    text_view,
                    findEditorTheme_(currentEditorThemePath_));
            });
    }

    virtual void postInit() override
    {
        // Window themes

        // TODO: Add user data paths to first arg
        auto window_theme_paths = Coco::PathUtil::fromDir(
            Coco::PathList{ THEMES_QRC_ },
            WindowTheme::EXT);

        for (auto& path : window_theme_paths)
            windowThemes_ << WindowTheme{ path };

        sortThemes_(windowThemes_);

        // Editor themes

        // TODO: Add user data paths to first arg
        auto editor_theme_paths = Coco::PathUtil::fromDir(
            Coco::PathList{ THEMES_QRC_ },
            EditorTheme::EXT);

        for (auto& path : editor_theme_paths)
            editorThemes_ << EditorTheme{ path };

        sortThemes_(editorThemes_);
    }

private:
    static constexpr auto THEMES_QRC_ = ":/themes/";

    QSet<Window*> windows_{};
    QList<WindowTheme> windowThemes_{};
    bool initialWindowThemeLoaded_ = false;
    Coco::Path currentWindowThemePath_{};

    QSet<TextFileView*> textFileViews_{};
    QList<EditorTheme> editorThemes_{};
    bool initialEditorThemeLoaded_ = false;
    Coco::Path currentEditorThemePath_{};

    void setup_()
    {
        //...
    }

    template <typename ThemeT> void sortThemes_(QList<ThemeT>& themes) const
    {
        std::sort(
            themes.begin(),
            themes.end(),
            [](const ThemeT& t1, const ThemeT& t2) {
                return t1.name().toLower() < t2.name().toLower();
            });
    }

    template <typename ThemeT>
    QList<std::pair<QString, Coco::Path>>
    themeData_(const QList<ThemeT>& themes) const
    {
        QList<std::pair<QString, Coco::Path>> result{};
        result.reserve(themes.size());

        for (auto& theme : themes)
            result << std::make_pair(theme.name(), theme.path());

        return result;
    }

    WindowTheme findWindowTheme_(const Coco::Path& path) const
    {
        if (path.isEmpty()) return {};

        for (auto& theme : windowThemes_)
            if (theme.path() == path) return theme;

        return {};
    }

    EditorTheme findEditorTheme_(const Coco::Path& path) const
    {
        if (path.isEmpty()) return {};

        for (auto& theme : editorThemes_)
            if (theme.path() == path) return theme;

        return {};
    }

    void applyWindowTheme_(Window* window, const WindowTheme& theme)
    {
        if (!window) return;

        // TODO
    }

    void applyEditorTheme_(TextFileView* textFileView, const EditorTheme& theme)
    {
        if (!textFileView) return;

        auto palette = theme.isValid() ? theme.palette() : QPalette{};
        textFileView->editor()->setPalette(palette);
    }

    // Find & apply window theme

private slots:
    // Empty or non-existent path results in no theming
    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::WINDOW_THEME) {

            currentWindowThemePath_ = value.value<Coco::Path>();

            auto theme = findWindowTheme_(currentWindowThemePath_);
            for (auto& window : windows_)
                applyWindowTheme_(window, theme);

        } else if (key == Ini::Keys::EDITOR_THEME) {

            currentEditorThemePath_ = value.value<Coco::Path>();

            auto theme = findEditorTheme_(currentEditorThemePath_);
            for (auto& view : textFileViews_)
                applyEditorTheme_(view, theme);
        }
    }
};

} // namespace Fernanda
