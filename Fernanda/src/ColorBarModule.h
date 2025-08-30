#pragma once

#include <QHash>
#include <QObject>

#include "Coco/Debug.h"

#include "ColorBar.h"
#include "Commander.h"
#include "EventBus.h"
#include "IFileModel.h"
#include "IService.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window ColorBars
class ColorBarModule : public IService
{
    Q_OBJECT

public:
    explicit ColorBarModule(
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

        // Connect to a first windows shown signal? How to handle startup?

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
        if (!window) return;
        if (auto color_bar = colorBars_[window]) color_bar->run(color);
    }

    void runAll_(ColorBar::Color color) const
    {
        for (auto& color_bar : colorBars_)
            if (color_bar) color_bar->run(color);
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
