/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QDockWidget>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QStatusBar>
#include <QToolButton>
#include <QVariant>
#include <QVariantMap>
#include <Qt>

#include "Coco/Path.h"

#include "AbstractService.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "TreeView.h"
#include "Window.h"

namespace Fernanda {

// Coordinator for Window TreeViews
class TreeViewService : public AbstractService
{
    Q_OBJECT

public:
    using ModelHook = std::function<QAbstractItemModel*()>;
    using RootIndexHook = std::function<QModelIndex()>;
    using DockWidgetHook = std::function<QWidget*(TreeView* mainTree, Window*)>;

    TreeViewService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~TreeViewService() override { TRACER; }

    DECLARE_HOOK_ACCESSORS(ModelHook, modelHook, setModelHook, modelHook_);

    DECLARE_HOOK_ACCESSORS(
        RootIndexHook,
        rootIndexHook,
        setRootIndexHook,
        rootIndexHook_);

    DECLARE_HOOK_ACCESSORS(
        DockWidgetHook,
        dockWidgetHook,
        setDockWidgetHook,
        dockWidgetHook_);

    QModelIndex currentIndex(Window* window) const
    {
        if (!window) return {};

        if (auto tree_view = treeViews_.value(window))
            return tree_view->currentIndex();

        return {};
    }

    void edit(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;

        if (auto tree_view = treeViews_[window]) {
            auto i = index.isValid() ? index : tree_view->currentIndex();
            if (!i.isValid()) return;
            tree_view->edit(i);
        }
    }

    bool isExpanded(Window* window, const QModelIndex& index) const
    {
        if (!window || !index.isValid()) return false;
        if (auto tree_view = treeViews_[window])
            return tree_view->isExpanded(index);
        return false;
    }

    void expand(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;

        if (auto tree_view = treeViews_[window]) {
            auto i = index.isValid() ? index : tree_view->currentIndex();
            if (!i.isValid()) return;
            tree_view->expand(i);
        }
    }

    void collapse(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;

        if (auto tree_view = treeViews_[window]) {
            auto i = index.isValid() ? index : tree_view->currentIndex();
            if (!i.isValid()) return;
            tree_view->collapse(i);
        }
    }

    void setHeadersHidden(bool hidden)
    {
        headersHidden_ = hidden;

        for (auto& tree_view : treeViews_)
            tree_view->setHeaderHidden(hidden);
    }

signals:
    void treeViewDoubleClicked(Window* context, const QModelIndex& index);
    void treeViewContextMenuRequested(
        Window* context,
        const QPoint& globalPos,
        const QModelIndex& index);

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &TreeViewService::onWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &TreeViewService::onWindowDestroyed_);
    }

private:
    QHash<Window*, TreeView*> treeViews_{};
    ModelHook modelHook_ = nullptr;
    RootIndexHook rootIndexHook_ = nullptr;
    DockWidgetHook dockWidgetHook_ = nullptr;

    bool headersHidden_ = false;

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

        tree_view->setHeaderHidden(headersHidden_);

        if (modelHook_ && rootIndexHook_) {
            if (auto model = modelHook_()) {
                tree_view->setModel(model);
                auto root_index = rootIndexHook_();
                if (root_index.isValid()) tree_view->setRootIndex(root_index);
            }
        }

        // Determine what goes in the dock
        QWidget* dock_contents = tree_view;
        if (dockWidgetHook_) dock_contents = dockWidgetHook_(tree_view, window);

        dock_widget->setWidget(dock_contents);
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
                INFO(
                    "Tree view double-clicked in [{}]: index [{}]",
                    window,
                    index);
                emit treeViewDoubleClicked(window, index);
            });

        connect(
            tree_view,
            &TreeView::customContextMenuRequested,
            this,
            [&, window, tree_view](const QPoint& pos) {
                if (!window || !tree_view) return;

                auto point = tree_view->mapToGlobal(pos);
                auto model_index = tree_view->indexAt(pos);
                INFO(
                    "Requesting context menu in [{}] at [{}] for index [{}]",
                    window,
                    point,
                    model_index);
                emit treeViewContextMenuRequested(window, point, model_index);
            });

        // TODO: Needed? Check that it actually works, too, since it decays to
        // QObject before emitting destroyed...
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
