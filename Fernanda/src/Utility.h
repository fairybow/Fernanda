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
#include <QMapIterator>
#include <QModelIndex>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Concepts.h"

#include "IFileModel.h"
#include "IFileView.h"
#include "SavePrompt.h"
#include "TabWidget.h"
#include "Window.h"

// Core, top-level utility functions

namespace Fernanda {

template <typename SlotT>
inline void timer(QObject* parent, int msecs, SlotT slot)
{
    QTimer::singleShot(msecs, parent, slot);
}

template <Coco::Concepts::QObjectPointer T> inline T to(QObject* object)
{
    return qobject_cast<T>(object);
}

template <Coco::Concepts::QObjectPointer T> inline T to(const QObject* object)
{
    return qobject_cast<T>(object);
}

template <typename T> inline T to(const QVariant& variant)
{
    return variant.value<T>();
}

template <typename T>
inline T
to(const QVariantMap& variantMap,
   const QString& key,
   const QVariant& defaultValue = {})
{
    return variantMap.value(key, defaultValue).value<T>();
}

template <typename T> inline QVariant toQVariant(const T& value)
{
    return QVariant::fromValue<T>(value);
}

// We almost always expect to use QPointer like a regular pointer (as per
// documentation), but it's easy to forget to use `get` when making a QVariant
// and expecting to cast the value back as the raw pointer
template <typename T> inline QVariant toQVariant(const QPointer<T>& value)
{
    return QVariant::fromValue<T*>(value.get());
}

// Maybe make this a query, for organizational/clarity purposes. This works
// fine, but feels off in terms of design. We also may want a treeView function!
inline TabWidget* tabWidget(Window* window)
{
    if (!window) return nullptr;
    return to<TabWidget*>(window->centralWidget());
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

inline QString toQString(SaveResult saveResult) noexcept
{
    switch (saveResult) {
    default:
    case SaveResult::NoOp:
        return "SaveResult::NoOp";
    case SaveResult::Success:
        return "SaveResult::Success";
    case SaveResult::Fail:
        return "SaveResult::Fail";
    }
}

inline QString toQString(SaveChoice saveChoice) noexcept
{
    switch (saveChoice) {
    default:
    case SaveChoice::Cancel:
        return "SaveChoice::Cancel";
    case SaveChoice::Save:
        return "SaveChoice::Save";
    case SaveChoice::Discard:
        return "SaveChoice::Discard";
    }
}

inline QString toQString(QObject* object)
{
    return object ? COCO_PTR_QSTR(object) : "nullptr";
}

inline QString toQString(const QVariant& variant)
{
    return variant.canConvert<QObject*>() ? toQString(variant.value<QObject*>())
                                          : variant.toString();
}

inline QString toQString(const QVariantMap& variantMap)
{
    if (variantMap.isEmpty()) return "{}";
    constexpr auto element_format = "{ %0, %1 }";
    constexpr auto outer_format = "{ %0 }";
    QStringList list{};
    QMapIterator<QString, QVariant> it(variantMap);

    while (it.hasNext()) {
        it.next();
        list
            << QString(element_format).arg(it.key()).arg(toQString(it.value()));
    }

    return QString(outer_format).arg(list.join(", "));
}

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

inline QModelIndex getItemModelRootIndex(QAbstractItemModel* model)
{
    return model->property("root").value<QModelIndex>();
}

inline void
storeItemModelRootIndex(QAbstractItemModel* model, const QModelIndex& index)
{
    model->setProperty("root", index);
}

} // namespace Fernanda
