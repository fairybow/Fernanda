/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <concepts>
#include <format>
#include <string>

#include <QDomElement>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Concepts.h"
#include "Coco/Path.h"

#include "ToString.h"

#define STRING_FORMATTER_(T, Conversion)                                       \
    template <> struct std::formatter<T> : std::formatter<std::string>         \
    {                                                                          \
        auto format(const T& x, format_context& ctx) const                     \
        {                                                                      \
            return std::formatter<std::string>::format(Conversion, ctx);       \
        }                                                                      \
    }

STRING_FORMATTER_(QString, x.toStdString());
STRING_FORMATTER_(Coco::Path, x.toString());
STRING_FORMATTER_(QStringList, Fernanda::toString(x));
STRING_FORMATTER_(QVariantMap, Fernanda::toString(x));
STRING_FORMATTER_(QVariant, Fernanda::toString(x));
STRING_FORMATTER_(QModelIndex, Fernanda::toString(x));
STRING_FORMATTER_(QDomElement, Fernanda::toString(x));

template <Coco::Concepts::QObjectPointer T>
struct std::formatter<T> : std::formatter<std::string>
{
    auto format(T object, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(
            Fernanda::toString(object),
            ctx);
    }
};

// TODO: Move to Coco and use cpp version check
template <typename TagT>
struct std::formatter<Coco::Bool<TagT>> : std::formatter<std::string>
{
    auto format(const Coco::Bool<TagT>& b, std::format_context& ctx) const
    {
        auto name = std::string(TagT::name()) + "::" + (b ? "Yes" : "No");
        return std::formatter<std::string>::format(name, ctx);
    }
};

#undef STRING_FORMATTER_
