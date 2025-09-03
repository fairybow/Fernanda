#pragma once

#include <QDockWidget>
#include <QHash>
#include <QObject>
#include <QStatusBar>
#include <QToolButton>

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
    // QHash<Window*, TreeView*> treeViews_{};

    void initialize_()
    {
        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            /// Set initial visibility and size based on settings later
            auto dock_widget = new QDockWidget(window);
            auto tree_view = new TreeView(dock_widget);
            dock_widget->setWidget(tree_view);
            window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

            window->resizeDocks(
                { dock_widget },
                { (window->width() / 3) },
                Qt::Horizontal);

            auto toggler = new QToolButton;
            auto status_bar = window->statusBar();
            status_bar->addPermanentWidget(toggler);
            connect(toggler, &QToolButton::pressed, this, [=] {
                if (dock_widget->isFloating()) {
                    dock_widget->setFloating(false);
                } else {
                    dock_widget->setVisible(!dock_widget->isVisible());
                }
            });

            connect(window, &Window::destroyed, this, [=] {
                if (!window) return;
                // treeViews_.remove(window);
            });
        });
    }
};

} // namespace Fernanda
