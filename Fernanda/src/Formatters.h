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

#include "Coco/Path.h"

#include "ToString.h"

//#define DEFAULT_PARSE_                                                         \
//    constexpr auto parse(format_parse_context& ctx)                            \
//    {                                                                          \
//        auto it = ctx.begin();                                                 \
//        while (it != ctx.end() && *it != '}') {                                \
//            ++it;                                                              \
//        }                                                                      \
//        return it;                                                             \
//    }

#define DEFAULT_PARSE_                                                         \
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

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
FORMATTER_(QString, x.toStdString());
FORMATTER_(Coco::Path, x.toString());

FORMATTER_(
    QVariant,
    x.toString()
        .toStdString()); // Won't work for something complex. Could use a custom
                         // converter that just calls variant.toString for
                         // everything except specified types (like Coco::Path,
                         // QObject pointers, etc)

// Sort of works now
template <typename T>
    requires std::is_base_of_v<QObject, std::remove_pointer_t<T>>
struct std::formatter<T, char> : std::formatter<std::string, char>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(T obj, std::format_context& ctx) const
    {
        std::string output;
        if constexpr (std::is_pointer_v<T>) {
            if (obj) {
                output = std::format(
                    "{}(\"{}\") at {}",
                    obj->metaObject()->className(),
                    obj->objectName().toStdString(),
                    static_cast<const void*>(obj));
            } else {
                output = "nullptr";
            }
        } else {
            output = std::format(
                "{}(\"{}\") at {}",
                obj.metaObject()->className(),
                obj.objectName().toStdString(),
                static_cast<const void*>(&obj));
        }
        return std::formatter<std::string, char>::format(output, ctx);
    }
};

//template <typename T>
//    requires std::derived_from<std::remove_cv_t<T>, QObject>
//struct std::formatter<T*>
//{
//    DEFAULT_PARSE_
//
//    auto format(T* x, format_context& ctx) const
//    {
//        return std::format_to(ctx.out(), "{}", FernandaTemp::toString(x));
//    }
//};
//
//// Add const T* version to handle const pointers
//template <typename T>
//    requires std::derived_from<std::remove_cv_t<T>, QObject>
//struct std::formatter<const T*>
//{
//    DEFAULT_PARSE_
//
//    auto format(const T* x, format_context& ctx) const
//    {
//        return std::format_to(ctx.out(), "{}", FernandaTemp::toString(x));
//    }
//};

#undef FORMATTER_
#undef DEFAULT_PARSE_
