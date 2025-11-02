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

#include "Bus.h"
#include "ColorBar.h"
#include "Commands.h"
#include "Debug.h"
#include "Enums.h"
#include "IService.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window ColorBars
// TODO: Commands for setting position
class ColorBarModule : public IService
{
    Q_OBJECT

public:
    ColorBarModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        setup_();
    }

    virtual ~ColorBarModule() override { TRACER; }

protected:
    // TODO: Could have commands to run all color bars and call that in save
    // functions instead of having slightly convoluted, overly-specific save
    // events (like windowSaveExecuted vs workspaceSaveExecuted)?
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(
            Commands::RUN_COLOR_BAR,
            [&](const Command& cmd) {
                auto color = cmd.param<ColorBar::Color>("color");
                cmd.context ? run_(cmd.context, color) : runAll_(color);
            });

        bus->addCommandHandler(Commands::BE_CUTE, [&] {
            runAll_(ColorBar::Color::Pastel);
        });
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &ColorBarModule::onWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &ColorBarModule::onWindowDestroyed_);
    }

private:
    QHash<Window*, ColorBar*> colorBars_{};

    void setup_()
    {
        //...
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
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        // ColorBar floats outside layouts
        colorBars_[window] = new ColorBar(window);
    }

    void onWindowDestroyed_(Window* window)
    {
        if (!window) return;
        colorBars_.remove(window);
    }
};

} // namespace Fernanda
