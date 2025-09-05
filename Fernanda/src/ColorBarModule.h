/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QObject>

#include "Coco/Debug.h"

#include "ColorBar.h"
#include "Commander.h"
#include "EventBus.h"
#include "IFileModel.h"
#include "IService.h"
#include "Utility.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window ColorBars
class ColorBarModule : public IService
{
    Q_OBJECT

public:
    ColorBarModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~ColorBarModule() override { COCO_TRACER; }

private:
    QHash<Window*, ColorBar*> colorBars_{};

    void initialize_()
    {
        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            // ColorBar floats outside layouts
            colorBars_[window] = new ColorBar(window);

            connect(window, &Window::destroyed, this, [=] {
                if (!window) return;
                colorBars_.remove(window);
            });
        });

        connect(eventBus, &EventBus::workspaceInitialized, this, [&] {
            timer(this, 1000, [&] { runAll_(ColorBar::Pastel); });
        });

        connect(
            eventBus,
            &EventBus::windowSaveExecuted,
            this,
            &ColorBarModule::onWindowSaveExecuted_);

        connect(
            eventBus,
            &EventBus::workspaceSaveExecuted,
            this,
            &ColorBarModule::onWorkspaceSaveExecuted_);
    }

    void run_(Window* window, ColorBar::Color color) const
    {
        if (!window || !window->isVisible()) return;
        if (auto color_bar = colorBars_[window]) color_bar->run(color);
    }

    void runAll_(ColorBar::Color color) const
    {
        for (auto it = colorBars_.begin(); it != colorBars_.end(); ++it) {
            auto window = it.key();
            if (!window || !window->isVisible()) continue;
            if (auto color_bar = it.value()) color_bar->run(color);
        }
    }

private slots:
    void onWindowSaveExecuted_(Window* window, SaveResult result) const
    {
        if (!window) return;

        switch (result) {
        default:
        case SaveResult::NoOp:
            return;
        case SaveResult::Success:
            run_(window, ColorBar::Green);
            return;
        case SaveResult::Fail:
            run_(window, ColorBar::Red);
            return;
        }
    }

    void onWorkspaceSaveExecuted_(SaveResult result) const
    {
        switch (result) {
        default:
        case SaveResult::NoOp:
            return;
        case SaveResult::Success:
            runAll_(ColorBar::Green);
            return;
        case SaveResult::Fail:
            runAll_(ColorBar::Red);
            return;
        }
    }
};

} // namespace Fernanda
