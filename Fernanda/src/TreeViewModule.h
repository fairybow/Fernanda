/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractItemModel>
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
#include "Utility.h"
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
        /// Make slot
        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            if (!window) return;

            /// Set initial visibility and size based on settings later
            auto dock_widget = new QDockWidget(window);
            auto tree_view = new TreeView(dock_widget);

            if (auto model = commander->call<QAbstractItemModel*>(
                Calls::NewTreeViewModel))
            {
                tree_view->setModel(model);
                if (auto root_index = getItemModelRootIndex(model);
                    root_index.isValid()) {
                    tree_view->setRootIndex(root_index);
                }
            }

            dock_widget->setWidget(tree_view);
            window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

            window->resizeDocks(
                { dock_widget },
                { (window->width() / 3) },
                Qt::Horizontal);

            // Should always be created
            if (auto status_bar = window->statusBar()) {
                auto toggler = new QToolButton;
                status_bar->addPermanentWidget(toggler);
                connect(toggler, &QToolButton::pressed, this, [=] {
                    if (dock_widget->isFloating()) {
                        dock_widget->setFloating(false);
                    } else {
                        dock_widget->setVisible(!dock_widget->isVisible());
                    }
                });
            }

            connect(window, &Window::destroyed, this, [=] {
                if (!window) return;
                // treeViews_.remove(window);
            });
        });
    }
};

} // namespace Fernanda
