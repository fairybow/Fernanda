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
#include <QFileSystemModel>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QStatusBar>
#include <QToolButton>
#include <QTreeView>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/Utility.h"

#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "IService.h"
#include "Utility.h"
#include "Window.h"

namespace Fernanda {

COCO_TRIVIAL_QCLASS(TreeView, QTreeView);

// Coordinator for Window TreeViews
class TreeViewModule : public IService
{
    Q_OBJECT

public:
    TreeViewModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        setup_();
    }

    virtual ~TreeViewModule() override { TRACER; }

    static QModelIndex modelRootIndex(QAbstractItemModel* model)
    {
        return model->property("root").value<QModelIndex>();
    }

    static void
    saveModelRootIndex(QAbstractItemModel* model, const QModelIndex& index)
    {
        model->setProperty("root", index);
    }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    // QHash<Window*, TreeView*> treeViews_{};
    // A set instead, perhaps, just for quick updates across all Workspace
    // TreeViews

    void setup_()
    {
        //...
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;

        // Break into multiple methods

        /// Set initial visibility and size based on settings later
        // auto dock_widget = new QDockWidget(window);
        // auto tree_view = new TreeView(dock_widget);

        // if (auto model =
        //         bus->call<QAbstractItemModel*>(PolyCmd::NEW_TREE_VIEW_MODEL))
        //         {
        //     tree_view->setModel(model);
        //     if (auto root_index = modelRootIndex(model);
        //     root_index.isValid()) {
        //         tree_view->setRootIndex(root_index);
        //     }
        // }

        // dock_widget->setWidget(tree_view);
        // window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

        // window->resizeDocks(
        //     { dock_widget },
        //     { (window->width() / 3) },
        //     Qt::Horizontal);

        // connect(
        //     tree_view,
        //     &TreeView::doubleClicked,
        //     this,
        //     [&, tree_view, window](const QModelIndex& index) {
        //         auto model = tree_view->model();
        //         if (!model) return;

        //        Coco::Path path{};

        //        if (auto fs_model = cast<QFileSystemModel*>(model)) {
        //            path = fs_model->filePath(index);
        //        } // else if (auto archive_model = to<ArchiveModel*>(model))
        //          // { path = archive_model->filePath(index);
        //        //}

        //        if (!path.isEmpty()) {
        //            bus->execute(
        //                PolyCmd::OPEN_FILE,
        //                { { "path", path.toQString() } },
        //                window);
        //        }
        //    });

        ///// Button

        //// Should always be created
        // if (auto status_bar = window->statusBar()) {
        //     auto toggler = new QToolButton;
        //     status_bar->addPermanentWidget(toggler);
        //     connect(toggler, &QToolButton::pressed, this, [=] {
        //         if (dock_widget->isFloating()) {
        //             dock_widget->setFloating(false);
        //         } else {
        //             dock_widget->setVisible(!dock_widget->isVisible());
        //         }
        //     });
        // }

        ///// Window clean up (maybe)

        // connect(window, &Window::destroyed, this, [=] {
        //     if (!window) return;
        //     // treeViews_.remove(window);
        // });
    }
};

} // namespace Fernanda
