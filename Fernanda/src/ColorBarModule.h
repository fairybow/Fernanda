#pragma once

#include <QHash>
#include <QObject>

#include "Coco/Debug.h"

#include "ColorBar.h"
#include "Commander.h"
#include "EventBus.h"
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
    }
};

} // namespace Fernanda
