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
#include "ProxyStyle.h"
#include "TextFileView.h"
#include "Themes.h"
#include "Window.h"

namespace Fernanda {

// TODO: Install watcher on theme paths for hot reload?
//
// Themes are JSON files with special extensions. All theme variables (for
// windows and editors) are optional. An invalid theme is a theme with no name
// and an empty values array
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

        connect(
            bus,
            &Bus::windowCreated,
            this,
            &StyleModule::onBusWindowCreated_);

        connect(
            bus,
            &Bus::fileViewCreated,
            this,
            &StyleModule::onBusFileViewCreated_);
    }

    virtual void postInit() override
    {
        // TODO: Add user data paths to first arg
        Coco::PathList source_paths{ ":/themes/" };

        // Window themes
        auto window_theme_paths =
            Coco::PathUtil::fromDir(source_paths, WindowTheme::EXT);

        for (auto& path : window_theme_paths)
            windowThemes_ << WindowTheme{ path };

        sortThemes_(windowThemes_);

        // Editor themes
        auto editor_theme_paths =
            Coco::PathUtil::fromDir(source_paths, EditorTheme::EXT);

        for (auto& path : editor_theme_paths)
            editorThemes_ << EditorTheme{ path };

        sortThemes_(editorThemes_);
    }

private:
    ProxyStyle* proxyStyle_ = new ProxyStyle;
    QSet<Window*> windows_{};
    QList<WindowTheme> windowThemes_{};
    bool initialWindowThemeLoaded_ = false;
    Coco::Path currentWindowThemePath_{};

    QSet<TextFileView*> textFileViews_{};
    QList<EditorTheme> editorThemes_{};
    bool initialEditorThemeLoaded_ = false;
    Coco::Path currentEditorThemePath_{};

    void setup_() { proxyStyle_->setParent(this); }

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

        auto theme_valid = theme.isValid();

        auto icon_color =
            theme_valid ? theme.iconColor() : ProxyStyle::defaultIconColor();
        proxyStyle_->setIconColor(icon_color);

        /// TODO STYLE: See applyEditorTheme_ comments
        auto qss = theme_valid ? theme.styleSheet() : QString{};
        window->setStyleSheet(qss);
    }

    void applyEditorTheme_(TextFileView* textFileView, const EditorTheme& theme)
    {
        if (!textFileView) return;
        auto editor = textFileView->editor();
        if (!editor) return;

        /// TODO STYLE: Ideal place/way to log QSS? (`INFO("Style sheet set for
        /// all editors [{}]", qss)`)

        /// TODO STYLE: We're calcuating QSS for each editor, I think? Don't
        /// wanna do that unless needed for hot reload or something)
        auto qss = theme.isValid() ? theme.styleSheet() : QString{};
        editor->setStyleSheet(qss);
    }

private slots:
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

    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;

        /// TODO STYLE: Now that we are only using ProxyStyle for icons and not
        /// palette, is the current code path still fine (widget calls static
        /// ProxyStyle function which casts widget's parents' window's style to
        /// PS lol)
        window->setStyle(proxyStyle_);
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

        applyWindowTheme_(window, findWindowTheme_(currentWindowThemePath_));
    }

    void onBusFileViewCreated_(AbstractFileView* fileView)
    {
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

        applyEditorTheme_(text_view, findEditorTheme_(currentEditorThemePath_));
    }
};

} // namespace Fernanda
