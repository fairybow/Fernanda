/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QObject>

#include "AbstractService.h"
#include "Bus.h"
#include "ColorBar.h"
#include "Debug.h"
#include "Ini.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window ColorBars
// TODO: Commands for setting position
class ColorBarModule : public AbstractService
{
    Q_OBJECT

public:
    ColorBarModule(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~ColorBarModule() override { TRACER; }

    void green(Window* window = nullptr) const { run(ColorBar::Green, window); }

    void red(Window* window = nullptr) const { run(ColorBar::Red, window); }

    void pastel(Window* window = nullptr) const
    {
        run(ColorBar::Pastel, window);
    }

    void run(ColorBar::Color color, Window* window = nullptr) const
    {
        if (colorBars_.isEmpty()) return;

        if (window) {
            if (!window->isVisible()) return;
            if (auto color_bar = colorBars_[window]) color_bar->run(color);
        } else {
            for (auto it = colorBars_.begin(); it != colorBars_.end(); ++it) {
                auto window = it.key();
                if (!window || !window->isVisible()) continue;
                if (auto color_bar = it.value()) color_bar->run(color);
            }
        }
    }

protected:
    // TODO: Could have commands to run all color bars and call that in save
    // functions instead of having slightly convoluted, overly-specific save
    // events (like windowSaveExecuted vs workspaceSaveExecuted)?
    virtual void registerBusCommands() override
    {
        // TODO: Could make Colors enum private and use string args?
        /*bus->addCommandHandler(
            Commands::RUN_COLOR_BAR,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                auto color = cmd.param<ColorBar::Color>("color");
                run(cmd.context, color);
            });

        bus->addCommandHandler(
            Commands::RUN_ALL_COLOR_BARS,
            [&](const Command& cmd) {
                auto color = cmd.param<ColorBar::Color>("color");
                runAll(color);
            });*/
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &ColorBarModule::onBusWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &ColorBarModule::onBusWindowDestroyed_);

        connect(
            bus,
            &Bus::settingChanged,
            this,
            &ColorBarModule::onBusSettingChanged_);
    }

private:
    QHash<Window*, ColorBar*> colorBars_{};

    void setup_()
    {
        //...
    }

    void applyInitialSettings_(ColorBar* colorBar)
    {
        if (!colorBar) return;

        colorBar->setActive(bus->call<bool>(
            Bus::GET_SETTING,
            { { "key", Ini::Keys::COLOR_BAR_ACTIVE },
              { "defaultValue", Ini::Defaults::colorBarActive() } }));

        colorBar->setPosition(bus->call<ColorBar::Position>(
            Bus::GET_SETTING,
            { { "key", Ini::Keys::COLOR_BAR_POSITION },
              { "defaultValue", Ini::Defaults::colorBarPosition() } }));
    }

    template <typename CallableT> void forEachColorBar_(CallableT&& callable)
    {
        for (auto& cb : colorBars_)
            if (cb) callable(cb);
    }

private slots:
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;

        // ColorBar floats outside layouts
        auto color_bar = new ColorBar(window);
        colorBars_[window] = color_bar;

        applyInitialSettings_(color_bar);
    }

    void onBusWindowDestroyed_(Window* window)
    {
        if (!window) return;
        colorBars_.remove(window);
    }

    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::COLOR_BAR_ACTIVE)
            forEachColorBar_(
                [&](ColorBar* cb) { cb->setActive(value.toBool()); });

        if (key == Ini::Keys::COLOR_BAR_POSITION)
            forEachColorBar_([&](ColorBar* cb) {
                cb->setPosition(value.value<ColorBar::Position>());
            });
    }
};

} // namespace Fernanda
