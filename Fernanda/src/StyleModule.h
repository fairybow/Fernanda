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

    // public theme setting method, maybe (will run through all editors in
    // tracking QSet and set the theme

protected:
    virtual void registerBusCommands() override
    {
        //
    }

    virtual void connectBusEvents() override
    {
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

        // Add a "no theme" option (TODO: See how this renders in combo box or
        // if it works without a name...)
        // TODO: OR, we could have no entry for this and a way to clear via the
        // public setter for theme (setEditorTheme({}) or whatever)
        editorThemes_ << EditorTheme{};

        for (auto& path : paths)
            editorThemes_ << EditorTheme{ path };

        std::sort(
            editorThemes_.begin(),
            editorThemes_.end(),
            [](const EditorTheme& et1, const EditorTheme& et2) {
                return et1.name().toLower() < et2.name().toLower();
            });

        /// TEST
        for (auto& ed : editorThemes_)
            INFO(
                "Theme added: {}!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
                ed.name());

        currentEditorThemePath_ = bus->call<Coco::Path>(
            Bus::GET_SETTING,
            { { "key", Ini::Keys::EDITOR_THEME },
              { "defaultValue", qVar(Ini::Defaults::editorTheme()) } });
    }

private:
    QSet<TextFileView*> textFileViews_{};
    QList<EditorTheme> editorThemes_{};
    Coco::Path currentEditorThemePath_{};

    void setup_()
    {
        //...
    }

    EditorTheme findEditorTheme_(const Coco::Path& path)
    {
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
};

} // namespace Fernanda
