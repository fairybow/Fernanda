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
#include <QModelIndex>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include "Coco/Concepts.h"

#include "IFileModel.h"
#include "IFileView.h"
#include "TabWidget.h"
#include "Window.h"

/// The QObject conversion functions should NOT be "to". It's a cast, not like
/// converting from QString or similar (toStdString, etc).

namespace Fernanda {

template <typename SlotT>
inline void timer(QObject* parent, int msecs, SlotT slot)
{
    QTimer::singleShot(msecs, parent, slot);
}

template <Coco::Concepts::QObjectPointer T> inline T cast(QObject* object)
{
    return qobject_cast<T>(object);
}

template <Coco::Concepts::QObjectPointer T> inline T cast(const QObject* object)
{
    return qobject_cast<T>(object);
}

template <typename T> inline T to(const QVariant& variant)
{
    return variant.value<T>();
}

template <typename T> inline QVariant toQVariant(const T& value)
{
    return QVariant::fromValue<T>(value);
}

namespace Util {

    // Maybe make this a query, for organizational/clarity purposes. This works
    // fine, but feels off in terms of design. We also may want a treeView
    // function!
    inline TabWidget* tabWidget(Window* window)
    {
        if (!window) return nullptr;
        return cast<TabWidget*>(window->centralWidget());
    }

    // ^ ditto
    inline int tabCount(Window* window)
    {
        if (auto tab_widget = tabWidget(window)) return tab_widget->count();
        return -1;
    }

    // ^ ditto
    inline IFileView* viewAt(Window* window, int index)
    {
        if (!window) return nullptr;
        auto tab_widget = tabWidget(window);
        if (!tab_widget) return nullptr;

        auto i = (index < 0) ? tab_widget->currentIndex() : index;
        if (i < 0 || i > tab_widget->count() - 1) return nullptr;

        return tab_widget->widgetAt<IFileView*>(i);
    }

    // ^ ditto
    inline IFileModel* modelAt(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view) return nullptr;
        return view->model();
    }

    // TODO: Move
    inline bool isMultiWindow(IFileModel* model, QSet<Window*> windows)
    {
        if (!model) return false;

        auto window_count = 0;

        for (auto window : windows) {
            for (auto i = 0; i < tabCount(window); ++i) {
                if (modelAt(window, i) == model) {
                    ++window_count;
                    if (window_count >= 2) return true; // Early exit
                    break; // Move to next window
                }
            }
        }

        return false;
    }

    // TODO: Move
    inline QModelIndex getItemModelRootIndex(QAbstractItemModel* model)
    {
        return model->property("root").value<QModelIndex>();
    }

    // TODO: Move
    inline void
    storeItemModelRootIndex(QAbstractItemModel* model, const QModelIndex& index)
    {
        model->setProperty("root", index);
    }
} // namespace Util

} // namespace Fernanda
