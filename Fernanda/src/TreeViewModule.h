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
#include <Qt>

#include "Coco/Path.h"
#include "Coco/Utility.h"

#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "IService.h"
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

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(
            Commands::RENAME_TREE_VIEW_INDEX,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                auto tree_view = treeViews_[cmd.context];
                auto index =
                    cmd.param<QModelIndex>("index", tree_view->currentIndex());
                if (!index.isValid()) return;
                tree_view->edit(index);
            });
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &TreeViewModule::onWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &TreeViewModule::onWindowDestroyed_);
    }

private:
    QHash<Window*, TreeView*> treeViews_{};

    void setup_()
    {
        //...
    }

    // TODO: Set initial visibility and size based on settings
    // TODO: Section
    void addTreeView_(Window* window)
    {
        if (!window) return;

        auto dock_widget = new QDockWidget(window);
        auto tree_view = new TreeView(dock_widget);
        treeViews_[window] = tree_view;

        tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
        tree_view->setEditTriggers(
            QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed); // F2 (standard)

        // Drag and drop
        tree_view->setDragEnabled(true);
        tree_view->setAcceptDrops(true);
        tree_view->setDropIndicatorShown(true);
        tree_view->setDragDropMode(QAbstractItemView::InternalMove);
        tree_view->setDefaultDropAction(Qt::MoveAction);

        if (auto model =
                bus->call<QAbstractItemModel*>(Commands::WS_TREE_VIEW_MODEL)) {
            tree_view->setModel(model);

            auto root_index =
                bus->call<QModelIndex>(Commands::WS_TREE_VIEW_ROOT_INDEX);

            if (root_index.isValid()) tree_view->setRootIndex(root_index);
        }

        dock_widget->setWidget(tree_view);
        window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

        window->resizeDocks(
            { dock_widget },
            { window->width() / 3 },
            Qt::Horizontal);

        connect(
            tree_view,
            &TreeView::doubleClicked,
            this,
            [&, window](const QModelIndex& index) {
                if (!window) return;
                emit bus->treeViewDoubleClicked(window, index);
            });

        connect(
            tree_view,
            &TreeView::customContextMenuRequested,
            this,
            [&, window, tree_view](const QPoint& pos) {
                if (!window || !tree_view) return;
                emit bus->treeViewContextMenuRequested(
                    window,
                    tree_view->mapToGlobal(pos),
                    tree_view->indexAt(pos));
            });

        // TODO: Needed?
        connect(tree_view, &TreeView::destroyed, this, [&, window] {
            if (!window) return;
            treeViews_.remove(window);
        });
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;

        // Break into multiple methods

        addTreeView_(window);

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
    }

    void onWindowDestroyed_(Window* window)
    {
        if (!window) return;
        treeViews_.remove(window);
    }
};

} // namespace Fernanda
