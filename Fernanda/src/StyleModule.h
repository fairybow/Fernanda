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

#include <QFileSystemWatcher>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "Coco/Path.h"
#include "Coco/Utility.h"

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

// TODO: Update SettingsDialog entries when a theme is added/removed or name
// changes (maybe)
// TODO: Combine common code
// TODO: Any gaps or redundancies, especially with the to QFSW slots?
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
        setupThemes_(windowThemes_, userWindowThemePaths_, WindowTheme::EXT);
        setupThemes_(editorThemes_, userEditorThemePaths_, EditorTheme::EXT);
        setupThemeWatches_();
    }

private:
    static constexpr auto QRC_DIR_ = ":/themes/";

    QSet<Window*> windows_{};
    StyleContext* windowsStyleContext_ = new StyleContext(this);
    QList<WindowTheme> windowThemes_{};
    bool initialWindowThemeLoaded_ = false;
    Coco::Path currentWindowThemePath_{};

    QSet<TextFileView*> textFileViews_{};
    QList<EditorTheme> editorThemes_{};
    bool initialEditorThemeLoaded_ = false;
    Coco::Path currentEditorThemePath_{};

    QFileSystemWatcher* userThemeWatcher_ = new QFileSystemWatcher(this);
    QSet<Coco::Path> userWindowThemePaths_{};
    QSet<Coco::Path> userEditorThemePaths_{};

    void setup_()
    {
        connect(
            userThemeWatcher_,
            &QFileSystemWatcher::fileChanged,
            this,
            &StyleModule::onThemeFileChanged_);

        connect(
            userThemeWatcher_,
            &QFileSystemWatcher::directoryChanged,
            this,
            &StyleModule::onThemeDirectoryChanged_);
    }

    template <typename ThemeT>
    void setupThemes_(
        QList<ThemeT>& themes,
        QSet<Coco::Path>& userThemePaths,
        const QString& ext)
    {
        auto qrc_paths =
            Coco::dirPaths(QRC_DIR_, ext, Coco::Recursive::No);

        auto user_paths =
            Coco::dirPaths(
            AppDirs::userThemes(),
            ext,
            Coco::Recursive::No);

        userThemePaths = { user_paths.begin(), user_paths.end() };

        for (auto& path : qrc_paths + user_paths)
            themes << ThemeT{ path };

        sortThemes_<ThemeT>(themes);
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

    void setupThemeWatches_()
    {
        // Watch user data directory for new/removed theme files
        auto qstr_user_dir = AppDirs::userThemes().toQString();
        if (!qstr_user_dir.isEmpty()) userThemeWatcher_->addPath(qstr_user_dir);

        // Watch individual theme files for content changes
        updateWatchedFiles_();
    }

    void updateWatchedFiles_()
    {
        // Clear all and re-add

        auto current_files = userThemeWatcher_->files();
        if (!current_files.isEmpty())
            userThemeWatcher_->removePaths(current_files);

        QStringList qstr_paths{};
        for (auto& path : userWindowThemePaths_)
            qstr_paths << path.toQString();
        for (auto& path : userEditorThemePaths_)
            qstr_paths << path.toQString();

        if (!qstr_paths.isEmpty()) userThemeWatcher_->addPaths(qstr_paths);
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

    template <typename ThemeT>
    void rebuildTheme_(QList<ThemeT>& themes, const Coco::Path& path)
    {
        themes.removeIf([&](const ThemeT& t) { return t.path() == path; });
        themes << ThemeT{ path }; // (May be invalid now, which is fine)
        sortThemes_<ThemeT>(themes);
    }

    void reapplyWindowThemeIfCurrent_(const Coco::Path& path)
    {
        if (path != currentWindowThemePath_) return;

        auto theme = findTheme_(windowThemes_, path);

        auto icon_color = theme.hasIconColor()
                              ? theme.iconColor()
                              : StyleContext::defaultIconColor();
        windowsStyleContext_->setIconColor(icon_color);

        for (auto& window : windows_) {
            if (!window) continue;
            window->setStyleSheet(theme.qss());
        }
    }

    void reapplyEditorThemeIfCurrent_(const Coco::Path& path)
    {
        if (path != currentEditorThemePath_) return;

        auto theme = findTheme_(editorThemes_, path);

        for (auto& view : textFileViews_) {
            if (!view) continue;
            auto editor = view->editor();
            if (!editor) continue;
            editor->setStyleSheet(theme.qss());
        }
    }

private slots:
    void onThemeFileChanged_(const QString& path)
    {
        Coco::Path theme_path = path;

        // If file no longer exists, let directoryChanged handle removal. If
        // current theme was deleted, stale QSS remains applied.
        if (!theme_path.exists()) return;

        // Re-add! Via Qt: Note: As a safety measure, many applications save an
        // open file by writing a new file and then deleting the old one. In
        // your slot function, you can check watcher.files().contains(path). If
        // it returns false, check whether the file still exists and then call
        // addPath() to continue watching it.
        userThemeWatcher_->addPath(path);

        if (path.endsWith(WindowTheme::EXT)) {
            // Only process if already tracking (new files handled by
            // directoryChanged)
            if (!userWindowThemePaths_.contains(theme_path)) return;
            rebuildTheme_(windowThemes_, theme_path);
            reapplyWindowThemeIfCurrent_(theme_path);

        } else if (path.endsWith(EditorTheme::EXT)) {
            if (!userEditorThemePaths_.contains(theme_path)) return;
            rebuildTheme_(editorThemes_, theme_path);
            reapplyEditorThemeIfCurrent_(theme_path);
        }
    }

    void onThemeDirectoryChanged_(const QString& path)
    {
        (void)path;

        // Re-scan for current files on disk
        auto current_window_paths = Coco::dirPaths(
            AppDirs::userThemes(),
            WindowTheme::EXT,
            Coco::Recursive::No);

        auto current_editor_paths = Coco::dirPaths(
            AppDirs::userThemes(),
            EditorTheme::EXT,
            Coco::Recursive::No);

        QSet<Coco::Path> current_window_set{ current_window_paths.begin(),
                                             current_window_paths.end() };
        QSet<Coco::Path> current_editor_set{ current_editor_paths.begin(),
                                             current_editor_paths.end() };

        // Diff against tracked paths
        auto added_window = current_window_set - userWindowThemePaths_;
        auto removed_window = userWindowThemePaths_ - current_window_set;
        auto added_editor = current_editor_set - userEditorThemePaths_;
        auto removed_editor = userEditorThemePaths_ - current_editor_set;

        // Apply removals
        for (auto& path : removed_window) {
            windowThemes_.removeIf(
                [&](const WindowTheme& t) { return t.path() == path; });
        }

        for (auto& path : removed_editor) {
            editorThemes_.removeIf(
                [&](const EditorTheme& t) { return t.path() == path; });
        }

        // Apply additions
        for (auto& path : added_window)
            windowThemes_ << WindowTheme{ path };

        for (auto& path : added_editor)
            editorThemes_ << EditorTheme{ path };

        // Update tracked paths
        userWindowThemePaths_ = current_window_set;
        userEditorThemePaths_ = current_editor_set;

        // Sort if changed
        if (!added_window.isEmpty() || !removed_window.isEmpty())
            sortThemes_(windowThemes_);

        if (!added_editor.isEmpty() || !removed_editor.isEmpty())
            sortThemes_(editorThemes_);

        // Sync file watches
        updateWatchedFiles_();

        // Reapply if current theme was just (re-)added
        if (added_window.contains(currentWindowThemePath_))
            reapplyWindowThemeIfCurrent_(currentWindowThemePath_);

        if (added_editor.contains(currentEditorThemePath_))
            reapplyEditorThemeIfCurrent_(currentEditorThemePath_);
    }

    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::WINDOW_THEME) {

            currentWindowThemePath_ = value.value<Coco::Path>();

            auto theme = findTheme_(windowThemes_, currentWindowThemePath_);

            auto icon_color = theme.hasIconColor()
                                  ? theme.iconColor()
                                  : StyleContext::defaultIconColor();
            windowsStyleContext_->setIconColor(icon_color);

            auto qss = theme.qss();
            INFO("Setting window QSS: [{}]", qss);

            for (auto& window : windows_) {
                if (!window) continue;
                window->setStyleSheet(qss);
            }

        } else if (key == Ini::Keys::EDITOR_THEME) {

            currentEditorThemePath_ = value.value<Coco::Path>();

            auto theme = findTheme_(editorThemes_, currentEditorThemePath_);
            auto qss = theme.qss();
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

        windowsStyleContext_->attach(window);
        windows_ << window;

        // TODO: Search use of Window::destroyed and replace with this (also
        // remove the note where I said only use Window::destroyed lol)
        connect(window, &QObject::destroyed, this, [&, window] {
            windows_.remove(window);
        });

        // Lazy-load current theme path if not yet loaded
        if (!initialWindowThemeLoaded_) {
            currentWindowThemePath_ = bus->call<Coco::Path>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::WINDOW_THEME },
                  { "defaultValue", qVar(Ini::Defaults::windowTheme()) } });

            initialWindowThemeLoaded_ = true;
        }

        auto theme = findTheme_(windowThemes_, currentWindowThemePath_);

        // Need to set this here in case a window is made before
        // windowsStyleContext_ has an icon color set (which happens at least on
        // the first window)
        auto icon_color = theme.hasIconColor()
                              ? theme.iconColor()
                              : StyleContext::defaultIconColor();
        windowsStyleContext_->setIconColor(icon_color);

        window->setStyleSheet(theme.qss());
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
            currentEditorThemePath_ = bus->call<Coco::Path>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_THEME },
                  { "defaultValue", qVar(Ini::Defaults::editorTheme()) } });

            initialEditorThemeLoaded_ = true;
        }

        auto theme = findTheme_(editorThemes_, currentEditorThemePath_);
        auto editor = text_view->editor();
        if (!editor) return;
        editor->setStyleSheet(theme.qss());
    }
};

} // namespace Fernanda
