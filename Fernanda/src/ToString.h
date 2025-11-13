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

#include <QDomAttr>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QMapIterator>
#include <QMetaObject>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Concepts.h"

#define TO_STD_(Type, ArgName)                                                 \
    inline std::string toString(const Type& ArgName)                           \
    {                                                                          \
        return toQString(ArgName).toStdString();                               \
    }

namespace Fernanda {

// Ptr cannot be nullptr
template <typename T> inline QString ptrAddress(const T* ptr)
{
    return QString::asprintf("%p", ptr);
}

// Ptr cannot be nullptr
template <typename T> inline QString qObjectPtrAddress(const T* ptr)
{
    return QString("%0(%1)")
        .arg(ptr->metaObject()->className())
        .arg(ptrAddress<T>(ptr));
}

// Ptr can be nullptr
template <Coco::Concepts::QObjectDerived T>
inline QString toQString(const T* ptr)
{
    return ptr ? qObjectPtrAddress<T>(ptr) : "nullptr";
}

inline QString toQString(const QModelIndex& index)
{
    if (!index.isValid()) return "QModelIndex(Invalid)";

    return QString("QModelIndex(row:%1, col:%2, %3)")
        .arg(index.row())
        .arg(index.column())
        .arg(ptrAddress(index.internalPointer()));
}

inline QString toQString(const QDomElement& element)
{
    if (element.isNull()) return "QDomElement(Null)";

    auto tag = element.tagName();
    auto attrs = element.attributes();

    if (attrs.isEmpty()) {
        return QString("QDomElement(\"<%1>\")").arg(tag);
    }

    QStringList attr_list{};
    for (auto i = 0; i < attrs.count(); ++i) {
        auto attr = attrs.item(i).toAttr();
        attr_list << QString("%1='%2'").arg(attr.name()).arg(attr.value());
    }

    return QString("QDomElement(<%1 %2>)").arg(tag).arg(attr_list.join(" "));
}

// TODO: Redo this. It's nasty.
inline QString toQString(const QVariant& variant)
{
    auto x = [](const QString& text, const QString& type = {}) {
        constexpr auto format = "QVariant(%0)";
        constexpr auto format_type = "QVariant(%0(%1))";
        return type.isEmpty() ? QString(format).arg(text)
                              : QString(format_type).arg(type).arg(text);
    };

    if (!variant.isValid()) return x("Invalid");
    if (variant.isNull()) return x("Null");

    if (variant.typeId() == QMetaType::Bool) // More specific than canConvert
        return x(variant.value<bool>() ? "true" : "false");

    if (variant.canConvert<Coco::Path>())
        return x(variant.value<Coco::Path>().toQString(), "Coco::Path");

    if (variant.canConvert<QModelIndex>())
        return x(toQString(variant.value<QModelIndex>()));

    if (variant.canConvert<QObject*>())
        return x(toQString(variant.value<QObject*>()));

    /*if (variant.canConvert<QStringList>())
        return x(variant.value<QStringList>().join(", "), "QStringList");*/

    // Fallback to generic conversion
    auto text = variant.toString();
    if (text.isEmpty()) return "Non-printable type";

    return QString("QVariant(\"%0\")").arg(text);
}

inline QString toQString(const QVariantMap& variantMap)
{
    if (variantMap.isEmpty()) return "QVariantMap{}";
    constexpr auto inner_format = "{ %0, %1 }";
    constexpr auto outer_format = "QVariantMap{ %0 }";
    QStringList list{};
    QMapIterator<QString, QVariant> it(variantMap);

    while (it.hasNext()) {
        it.next();
        list << QString(inner_format).arg(it.key()).arg(toQString(it.value()));
    }

    return QString(outer_format).arg(list.join(", "));
}

// Std

// Ptr can be nullptr
template <Coco::Concepts::QObjectDerived T>
inline std::string toString(const T* ptr)
{
    return toQString<T>(ptr).toStdString();
}

TO_STD_(QModelIndex, index);
TO_STD_(QDomElement, element);
TO_STD_(QVariant, variant);
TO_STD_(QVariantMap, variantMap);

} // namespace Fernanda

#undef TO_STD_
