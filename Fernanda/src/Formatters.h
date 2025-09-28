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

//#include "IFileModel.h"
//#include "SavePrompt.h"
#include "ToString.h"

#define DEFAULT_PARSE_                                                         \
    constexpr auto parse(format_parse_context& ctx)                            \
    {                                                                          \
        auto it = ctx.begin();                                                 \
        while (it != ctx.end() && *it != '}') {                                \
            ++it;                                                              \
        }                                                                      \
        return it;                                                             \
    }

// Are these any different?

// Also, should I inherit formatter for std::string instead?

//#define DEFAULT_PARSE_                                                         \
//    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

#define FORMATTER_(T, Conversion)                                              \
    template <> struct std::formatter<T>                                       \
    {                                                                          \
        DEFAULT_PARSE_                                                         \
        auto format(const T& x, format_context& ctx) const                     \
        {                                                                      \
            return std::format_to(ctx.out(), "{}", Conversion);                \
        }                                                                      \
    }

FORMATTER_(QVariantMap, FernandaTemp::toString(x));
//FORMATTER_(SaveResult, FernandaTemp::toString(x));
//FORMATTER_(SaveChoice, FernandaTemp::toString(x));
FORMATTER_(QString, x.toStdString());
FORMATTER_(Coco::Path, x.toString());

FORMATTER_(
    QVariant,
    x.toString()
        .toStdString()); // Won't work for something complex. Could use a custom
                         // converter that just calls variant.toString for
                         // everything except specified types (like Coco::Path,
                         // QObject pointers, etc): i.e.,
                         // `FernandaTemp::toString(x)`

template <typename T>
    requires Coco::Concepts::QObjectPointer<T>
struct std::formatter<T>
{
    DEFAULT_PARSE_

    auto format(T object, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", FernandaTemp::toString(object));
    }
};

#undef FORMATTER_
#undef DEFAULT_PARSE_
