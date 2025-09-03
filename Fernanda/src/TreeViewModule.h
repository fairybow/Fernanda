#pragma once

#include <QHash>
#include <QObject>

#include "Coco/Debug.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "TreeView.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window TreeViews
class TreeViewModule : public IService
{
    Q_OBJECT

public:
    TreeViewModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~TreeViewModule() override { COCO_TRACER; }

private:
    QHash<Window*, TreeView*> treeViews_{};

    void initialize_()
    {
        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            // Wrap in a QDockWidget

            // ColorBar floats outside layouts
            // colorBars_[window] = new ColorBar(window);

            connect(window, &Window::destroyed, this, [=] {
                if (!window) return;
                // colorBars_.remove(window);
            });
        });
    }
};

} // namespace Fernanda
