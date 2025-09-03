#pragma once

#include <QHash>
#include <QObject>
#include <QDockWidget>

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
    //QHash<Window*, TreeView*> treeViews_{};

    void initialize_()
    {
        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            auto dock_widget = new QDockWidget(window);
            auto tree_view = new TreeView(dock_widget);
            dock_widget->setWidget(tree_view);
            window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

            window->resizeDocks(
                { dock_widget },
                { (window->width() / 3) },
                Qt::Horizontal);

            connect(window, &Window::destroyed, this, [=] {
                if (!window) return;
                // treeViews_.remove(window);
            });
        });
    }
};

} // namespace Fernanda
