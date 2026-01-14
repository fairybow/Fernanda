/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

#include "Debug.h"
#include "Io.h"

namespace Fernanda {

enum class UiIcon
{
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronUp,
    Dot,
    Plus,
    X
};

class ProxyStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit ProxyStyle(QStyle* style = nullptr)
        : QProxyStyle(style)
    {
        setup_();
    }

    virtual ~ProxyStyle() override { TRACER; }

    QColor iconColor() const { return iconColor_; }

    void setIconColor(const QColor& color)
    {
        if (iconColor_ == color) return;
        iconColor_ = color;
        clearCache_();
    }

    QPixmap icon(UiIcon icon, const QSize& size, qreal dpr) const
    {
        if (!size.isValid()) return {};

        auto key = cacheKey_(icon, size, dpr);
        if (cache_.contains(key)) return cache_[key];

        auto path = registry_.value(icon);
        if (path.isEmpty()) FATAL("Path not set for icon [{}]", icon);

        auto pixmap = renderSvg_(path, size, dpr);
        if (!pixmap.isNull()) cache_[key] = pixmap;
        return pixmap;
    }

private:
    static constexpr auto PLACEHOLDER_COLOR_ = "#404040";

    QColor iconColor_{ PLACEHOLDER_COLOR_ };
    QHash<UiIcon, QString> registry_{};
    mutable QHash<QString, QPixmap> cache_{};

    void setup_()
    {
        registry_[UiIcon::ChevronDown] = ":/ui/ChevronDown.svg";
        registry_[UiIcon::ChevronLeft] = ":/ui/ChevronLeft.svg";
        registry_[UiIcon::ChevronRight] = ":/ui/ChevronRight.svg";
        registry_[UiIcon::ChevronUp] = ":/ui/ChevronUp.svg";
        registry_[UiIcon::Dot] = ":/ui/Dot.svg";
        registry_[UiIcon::Plus] = ":/ui/Plus.svg";
        registry_[UiIcon::X] = ":/ui/X.svg";
    }

    void clearCache_() { cache_.clear(); }

    QString cacheKey_(UiIcon icon, const QSize& size, qreal dpr) const
    {
        return QString("%1_%2x%3@%4_%5")
            .arg(static_cast<int>(icon))
            .arg(size.width())
            .arg(size.height())
            .arg(dpr)
            .arg(iconColor_.name());
    }

    QPixmap renderSvg_(const QString& path, const QSize& size, qreal dpr) const
    {
        auto data = Io::read(path);
        if (data.isEmpty()) {
            WARN("Failed to read SVG [{}]", path);
            return {};
        }

        // Colorize
        auto color_hex = iconColor_.name().toUtf8(); // "#rrggbb"
        data.replace(PLACEHOLDER_COLOR_, color_hex);

        QSvgRenderer renderer(data);
        if (!renderer.isValid()) {
            WARN("Invalid SVG [{}]", path);
            return {};
        }

        QPixmap pixmap(size * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        renderer.render(&painter);

        return pixmap;
    }
};

} // namespace Fernanda
