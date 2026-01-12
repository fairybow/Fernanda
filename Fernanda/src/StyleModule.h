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
        bus->addCommandHandler(Bus::EDITOR_THEMES, [&] {
            QList<std::pair<QString, Coco::Path>> result{};
            result.reserve(editorThemes_.size());

            for (auto& theme : editorThemes_)
                result << std::make_pair(theme.name(), theme.path());

            return result;
        });

        bus->addCommandHandler(Bus::WINDOW_THEMES, [&] {
            QList<std::pair<QString, Coco::Path>> result{};

            //...

            return result;
        });
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::settingChanged,
            this,
            &StyleModule::onBusSettingChanged_);

        // TODO: Connect new window / lazy load theme

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
        // TODO: Add user data paths to first arg
        auto paths = Coco::PathUtil::fromDir(
            Coco::PathList{ ":/themes/" },
            EditorTheme::EXT);

        for (auto& path : paths)
            editorThemes_ << EditorTheme{ path };

        std::sort(
            editorThemes_.begin(),
            editorThemes_.end(),
            [](const EditorTheme& et1, const EditorTheme& et2) {
                return et1.name().toLower() < et2.name().toLower();
            });

        // TODO: Window themes
    }

private:
    QSet<TextFileView*> textFileViews_{};
    QList<EditorTheme> editorThemes_{};
    bool initialEditorThemeLoaded_ = false;
    Coco::Path currentEditorThemePath_{};

    void setup_()
    {
        //...
    }

    EditorTheme findEditorTheme_(const Coco::Path& path)
    {
        if (path.isEmpty()) return {};

        for (auto& theme : editorThemes_)
            if (theme.path() == path) return theme;

        return {};
    }

    void applyEditorTheme_(TextFileView* textFileView, const EditorTheme& theme)
    {
        if (!textFileView) return;

        auto palette = theme.isValid() ? theme.palette() : QPalette{};
        textFileView->editor()->setPalette(palette);
    }

    // Find & apply window theme

    /*
    // Example editor theme (base window theme off this):
    {
      "name": "Pocket",
      "values": {
        "backgroundColor": "#b6bc9f",
        "fontColor": "#1b211b",
        "selectionBgColor": "#1b211b",
        "selectionFontColor": "#b6bc9f"
       }
    }
    */

private slots:
    // Empty or non-existent path results in no theming
    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        // TODO: Handle window theme

        if (key != Ini::Keys::EDITOR_THEME) return;

        currentEditorThemePath_ = value.value<Coco::Path>();

        auto theme = findEditorTheme_(currentEditorThemePath_);
        for (auto& view : textFileViews_)
            applyEditorTheme_(view, theme);
    }
};

} // namespace Fernanda
