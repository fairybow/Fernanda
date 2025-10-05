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

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Concepts.h"
#include "Coco/Path.h"

#include "ToString.h"

// NOTE: Unused
#define DEFAULT_PARSE_                                                         \
    constexpr auto parse(format_parse_context& ctx)                            \
    {                                                                          \
        auto it = ctx.begin();                                                 \
        while (it != ctx.end() && *it != '}') {                                \
            ++it;                                                              \
        }                                                                      \
        return it;                                                             \
    }

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
STRING_FORMATTER_(QVariantMap, Fernanda::toString(x));

// TODO: Won't work for something complex. Could use a custom
// converter that just calls variant.toString for
// everything except specified types (like Coco::Path,
// QObject pointers, etc): i.e.,
// `FernandaTemp::toString(x)`
STRING_FORMATTER_(QVariant, x.toString().toStdString());

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

#undef STRING_FORMATTER_
#undef DEFAULT_PARSE_
