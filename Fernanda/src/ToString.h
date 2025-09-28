/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <string>

#include <QMapIterator>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Concepts.h"

#include "Enums.h"

namespace Fernanda {

template <Coco::Concepts::QObjectDerived T> inline QString toQString(T* ptr)
{
    if (ptr) {
        return QString("%0(%1)")
            .arg(ptr->metaObject()->className())
            .arg(QString::asprintf("%p", ptr));
    }

    return "nullptr";
}

template <Coco::Concepts::QObjectDerived T>
inline QString toQString(const T* ptr)
{
    if (ptr) {
        return QString("%0(%1)")
            .arg(ptr->metaObject()->className())
            .arg(QString::asprintf("%p", ptr));
    }

    return "nullptr";
}

template <Coco::Concepts::QObjectDerived T> inline std::string toString(T* ptr)
{
    return toQString<T>(ptr).toStdString();
}

template <Coco::Concepts::QObjectDerived T>
inline std::string toString(const T* ptr)
{
    return toQString<T>(ptr).toStdString();
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
        list << QString(element_format)
                    .arg(it.key())
                    .arg(it.value().toString()); /// Use custom converter later
                                                 /// maybe for QVar
    }

    return QString(outer_format).arg(list.join(", "));
}

inline std::string toString(const QVariantMap& variantMap)
{
    return toQString(variantMap).toStdString();
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

inline std::string toString(SaveResult saveResult) noexcept
{
    return toQString(saveResult).toStdString();
}

inline std::string toString(SaveChoice saveChoice) noexcept
{
    return toQString(saveChoice).toStdString();
}

} // namespace Fernanda
