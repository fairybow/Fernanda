/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QList>
#include <QObject>
#include <QSet>

#include "AbstractFileView.h"
#include "AbstractService.h"
#include "Bus.h"
#include "Debug.h"
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
                // If view is TextFileView, add to tracking QSet
                // If view is TextFileView, somehow apply current theme to the
                // editor
            });

        connect(
            bus,
            &Bus::fileViewDestroyed,
            this,
            [&](AbstractFileView* fileView) {
                // Remove from tracking QSet
            });
    }

    // public theme setting method, maybe (will run through all editors in
    // tracking QSet and set the theme

private:
    // QSet<> editors_{};

    qsizetype currentEditorTheme_ =
        0; /// TODO STYLE: retrieve from settings - may want to do it by name,
           /// since number of themes can change - when name doesn't match
           /// anything, use no theme
    QList<EditorTheme> editorThemes_{};

    void setup_()
    {
        // Add "no theme"
        // editorThemes_ << EditorTheme{};
        editorThemes_ << EditorTheme{
            ":/themes/Pocket.fernanda_editor"
        }; /// <-testing, Pocket as default

        // Later: Read QRC + user data for theme files and parse and store
    }
};

} // namespace Fernanda
