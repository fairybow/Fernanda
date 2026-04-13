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

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

#include "core/Debug.h"
#include "core/Io.h"

namespace Fernanda {

enum class UiIcon
{
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronUp,
    Dot,
    Minus,
    Plus,
    X
};

namespace Icons {

    using namespace Qt::StringLiterals;

    namespace Internal {

        inline constexpr auto PLACEHOLDER_COLOR_ = "#00ff00";

        inline const QString& path_(UiIcon type)
        {
            static const QHash<UiIcon, QString> paths{
                { UiIcon::ChevronDown, u":/ui/ChevronDown.svg"_s },
                { UiIcon::ChevronLeft, u":/ui/ChevronLeft.svg"_s },
                { UiIcon::ChevronRight, u":/ui/ChevronRight.svg"_s },
                { UiIcon::ChevronUp, u":/ui/ChevronUp.svg"_s },
                { UiIcon::Dot, u":/ui/Dot.svg"_s },
                { UiIcon::Minus, u":/ui/Minus.svg"_s },
                { UiIcon::Plus, u":/ui/Plus.svg"_s },
                { UiIcon::X, u":/ui/X.svg"_s },
            };

            auto it = paths.find(type);
            ASSERT(
                it != paths.end(),
                "No path for icon [{}]",
                static_cast<int>(type));

            return it.value();
        }

    } // namespace Internal

    inline QPixmap
    get(UiIcon type,
        const QSize& size,
        const QColor& color = Qt::white,
        qreal dpr = 1.0)
    {
        if (!size.isValid()) return {};

        auto data = Io::read(Internal::path_(type));
        if (data.isEmpty()) {
            WARN("Failed to read SVG [{}]", Internal::path_(type));
            return {};
        }

        data.replace(Internal::PLACEHOLDER_COLOR_, color.name().toUtf8());

        QSvgRenderer renderer(data);
        if (!renderer.isValid()) {
            WARN("Invalid SVG [{}]", Internal::path_(type));
            return {};
        }

        QPixmap pixmap(size * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        renderer.render(&painter, QRectF({ 0, 0 }, size));

        return pixmap;
    }

} // namespace Icons

} // namespace Fernanda
