/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include <concepts>
#include <format>
#include <string>

#include <QDomElement>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <Coco/Concepts.h>

#include "core/ToString.h"

#define STRING_FORMATTER_(T, Conversion)                                       \
    template <> struct std::formatter<T> : std::formatter<std::string>         \
    {                                                                          \
        auto format(const T& x, format_context& ctx) const                     \
        {                                                                      \
            return std::formatter<std::string>::format(Conversion, ctx);       \
        }                                                                      \
    }

STRING_FORMATTER_(QString, x.toStdString());
STRING_FORMATTER_(QStringList, Fernanda::toString(x));
STRING_FORMATTER_(QVariantMap, Fernanda::toString(x));
STRING_FORMATTER_(QVariant, Fernanda::toString(x));
STRING_FORMATTER_(QModelIndex, Fernanda::toString(x));
STRING_FORMATTER_(QPoint, Fernanda::toString(x));
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

#undef STRING_FORMATTER_
