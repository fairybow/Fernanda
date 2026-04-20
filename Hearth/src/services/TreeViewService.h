/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QAction>
#include <QDockWidget>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QStatusBar>
#include <QString>
#include <QToolButton>
#include <QVariant>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Tr.h"
#include "services/AbstractService.h"
#include "ui/TreeView.h"
#include "ui/Window.h"
#include "workspaces/Bus.h"

namespace Fernanda {

// Coordinator for Window TreeViews
// TODO: Ensure we have appropriate names for members/functions! If it deals
// with the dock, instead of the TreeView directly, ensure it says so on the
// tin.
// TODO: Potentially rename to TreeViewDock or make a composite widget here? We
// need clarity around these two components
// TODO: Second-level indent is way too deep
class TreeViewService : public AbstractService
{
    Q_OBJECT

public:
    TreeViewService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~TreeViewService() override { TRACER; }

    // Set broadly in Workspace
    DECLARE_HOOK(std::function<QAbstractItemModel*()>, modelHook, setModelHook)

    // Set broadly in Workspace
    DECLARE_HOOK(std::function<QModelIndex()>, rootIndexHook, setRootIndexHook)

    // Set per-Workspace
    DECLARE_HOOK(
        std::function<QWidget*(TreeView*, Window*)>,
        dockWidgetHook,
        setDockWidgetHook)

    // Per-window dock API

    QModelIndex currentIndex(Window* window) const
    {
        if (!window) return {};

        if (auto tree_view = treeViews_.value(window))
            return tree_view->currentIndex();

        return {};
    }

    void setCurrentIndex(Window* window, const QModelIndex& index)
    {
        if (!window) return;

        if (auto tree_view = treeViews_.value(window))
            tree_view->setCurrentIndex(index);
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

    void setDockVisible(Window* window, bool visible)
    {
        if (auto dock_widget = dockWidgets_.value(window))
            dock_widget->setVisible(visible);
    }

    QAction* dockToggleViewAction(Window* window) const
    {
        if (auto dock = dockWidgets_.value(window))
            return dock->toggleViewAction();

        return nullptr;
    }

    // All docks API

    void setDockWidgetFeatures(QDockWidget::DockWidgetFeatures features)
    {
        dockWidgetFeatures_ = features;

        for (auto& dock_widget : dockWidgets_)
            dock_widget->setFeatures(features);
    }

    void setHeadersHidden(bool hidden)
    {
        headersHidden_ = hidden;

        for (auto& tree_view : treeViews_) {
            tree_view->setHeaderHidden(hidden);
        }
    }

    void setVisibilityKey(const QString& iniKey) { visibilityIniKey_ = iniKey; }

signals:
    void doubleClicked(Window* context, const QModelIndex& index);
    void contextMenuRequested(
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
            &TreeViewService::onBusWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &TreeViewService::onBusWindowDestroyed_);
    }

private:
    QHash<Window*, QDockWidget*> dockWidgets_{};
    QHash<Window*, TreeView*> treeViews_{};
    QDockWidget::DockWidgetFeatures dockWidgetFeatures_{
        QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
        | QDockWidget::DockWidgetFloatable
    };

    bool headersHidden_ = false;

    QString visibilityIniKey_{};

    void setup_()
    {
        //...
    }

    // TODO: Set initial size based on settings
    // TODO: Break into smaller functions
    void addTreeView_(Window* window)
    {
        if (!window) return;

        auto dock_widget = new QDockWidget(window);
        dockWidgets_[window] = dock_widget;

        dock_widget->toggleViewAction()->setText(Tr::nxTreeView());
        dock_widget->setVisible(bus->call<bool>(
            Bus::GET_SETTING,
            { { "key", visibilityIniKey_ } }));

        // TODO: Needed? Check that it actually works, too, since it decays to
        // QObject before emitting destroyed...
        connect(dock_widget, &QObject::destroyed, this, [this, window] {
            // if (!window) return;
            dockWidgets_.remove(window);
        });

        auto tree_view = new TreeView(dock_widget);

        // TODO: Tracking/clean-up helper
        treeViews_[window] = tree_view;
        connect(tree_view, &QObject::destroyed, this, [this, window] {
            // if (!window) return;
            treeViews_.remove(window);
        });

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
        dock_widget->setFeatures(dockWidgetFeatures_);
        window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

        window->resizeDocks(
            { dock_widget },
            { window->width() / 3 },
            Qt::Horizontal);

        connect(
            tree_view,
            &TreeView::doubleClicked,
            this,
            [this, window](const QModelIndex& index) {
                if (!window) return;
                INFO(
                    "Tree view double-clicked in [{}]: index [{}]",
                    window,
                    index);
                emit doubleClicked(window, index);
            });

        connect(
            tree_view,
            &TreeView::customContextMenuRequested,
            this,
            [this, window, tree_view](const QPoint& pos) {
                if (!window || !tree_view) return;

                auto point = tree_view->mapToGlobal(pos);
                auto model_index = tree_view->indexAt(pos);
                INFO(
                    "Requesting context menu in [{}] at [{}] for index [{}]",
                    window,
                    point,
                    model_index);
                emit contextMenuRequested(window, point, model_index);
            });
    }

private slots:
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;
        addTreeView_(window);
    }

    void onBusWindowDestroyed_(Window* window)
    {
        if (!window) return;

        dockWidgets_.remove(window);
        treeViews_.remove(window);
    }
};

} // namespace Fernanda
