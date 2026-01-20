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
#include "AppDirs.h"
#include "Bus.h"
#include "Debug.h"
#include "Ini.h"
#include "StyleContext.h"
#include "TextFileView.h"
#include "Themes.h"
#include "Window.h"

namespace Fernanda {

// TODO: Install watcher on theme paths for hot reload?
// TODO: Combine common code
//
// Themes are JSON files with special extensions. All theme variables (for
// windows and editors) are optional. An invalid theme is a theme with no name
// and an empty values array (or default constructed)
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
        Coco::PathList source_paths{ ":/themes/", AppDirs::userData() };

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
    StyleContext* styleContext_ = new StyleContext(this);

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
        //
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

    template <typename ThemeT>
    ThemeT findTheme_(const QList<ThemeT>& themes, const Coco::Path& path) const
    {
        if (path.isEmpty()) return {};

        for (auto& theme : themes)
            if (theme.path() == path) return theme;

        return {};
    }

private slots:
    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::WINDOW_THEME) {

            currentWindowThemePath_ = value.value<Coco::Path>();

            auto theme = findTheme_(windowThemes_, currentWindowThemePath_);
            auto theme_valid = theme.isValid();

            auto icon_color = theme_valid ? theme.iconColor()
                                          : StyleContext::defaultIconColor();
            styleContext_->setIconColor(icon_color);

            // Don't really need to do this, since an invalid theme will return
            // an empty QString anyway, but perhaps its sensible...
            auto qss = theme_valid ? theme.qss() : QString{};
            INFO("Setting window QSS: [{}]", qss);

            for (auto& window : windows_) {
                if (!window) continue;
                window->setStyleSheet(qss);
            }

        } else if (key == Ini::Keys::EDITOR_THEME) {

            currentEditorThemePath_ = value.value<Coco::Path>();

            auto theme = findTheme_(editorThemes_, currentEditorThemePath_);
            auto qss = theme.isValid() ? theme.qss() : QString{};
            INFO("Setting editor QSS: [{}]", qss);

            for (auto& view : textFileViews_) {
                if (!view) continue;
                auto editor = view->editor();
                if (!editor) continue;

                editor->setStyleSheet(qss);
            }
        }
    }

    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;

        styleContext_->attach(window);
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

        auto theme = findTheme_(windowThemes_, currentWindowThemePath_);
        auto theme_valid = theme.isValid();

        // Need to set this here in case a window is made before styleContext_
        // has an icon color set (which happens at least on the first window)
        auto icon_color =
            theme_valid ? theme.iconColor() : StyleContext::defaultIconColor();
        styleContext_->setIconColor(icon_color);

        auto qss = theme_valid ? theme.qss() : QString{};
        window->setStyleSheet(qss);
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

        auto theme = findTheme_(editorThemes_, currentEditorThemePath_);
        auto qss = theme.isValid() ? theme.qss() : QString{};
        auto editor = text_view->editor();
        if (!editor) return;
        editor->setStyleSheet(qss);
    }
};

} // namespace Fernanda
