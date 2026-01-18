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
#include <QMenuBar>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QProxyStyle>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QSvgRenderer>
#include <QWidget>

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

    static QColor defaultIconColor() { return PLACEHOLDER_COLOR_; }
    QColor iconColor() const { return iconColor_; }

    // Used by StyleModule
    void setIconColor(const QColor& color)
    {
        if (iconColor_ == color) return;
        iconColor_ = color;
        cache_.clear();

        for (auto& requester : iconRequesters_)
            requester->update();
    }

    // For style-aware, from-SVG icons
    static QPixmap icon(QWidget* widget, UiIcon type, const QSize& size)
    {
        if (!widget) return {};
        auto window = widget->window();
        if (!window) return {};

        auto ps = qobject_cast<ProxyStyle*>(window->style());
        if (!ps) return {};

        return ps->icon_(widget, type, size, widget->devicePixelRatio());
    }

    // Used by StyleModule
    /// TODO STYLE: track and style context menus
    void setMenuStyleSheet(const QString& styleSheet)
    {
        if (menuStyleSheet_ == styleSheet) return;
        menuStyleSheet_ = styleSheet;

        for (auto& requester : menuStyleSheetRequesters_)
            requester->setStyleSheet(menuStyleSheet_);
    }

    /// TODO STYLE: track and style context menus
    static void trackAndStyle(QMenuBar* menuBar)
    {
        if (!menuBar) return;
        auto window = menuBar->window();
        if (!window) return;

        auto ps = qobject_cast<ProxyStyle*>(window->style());
        if (!ps) return;

        ps->trackAndStyle_(menuBar);
    }

private:
    static constexpr auto PLACEHOLDER_COLOR_ = "#404040";

    QColor iconColor_{ PLACEHOLDER_COLOR_ };
    QHash<UiIcon, QString> registry_{};
    mutable QHash<QString, QPixmap> cache_{};
    mutable QSet<QWidget*> iconRequesters_{};

    /// TODO STYLE
    QString menuStyleSheet_{};
    mutable QSet<QWidget*> menuStyleSheetRequesters_{};

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

    QString cacheKey_(UiIcon type, const QSize& size, qreal dpr) const
    {
        return QString("%1_%2x%3@%4_%5")
            .arg(static_cast<int>(type))
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
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Render to logical bounds
        renderer.render(&painter, QRectF({ 0, 0 }, size));

        return pixmap;
    }

    QPixmap
    icon_(QWidget* requester, UiIcon type, const QSize& size, qreal dpr) const
    {
        if (!requester || !size.isValid()) return {};

        if (!iconRequesters_.contains(requester)) {
            iconRequesters_ << requester;
            connect(requester, &QObject::destroyed, this, [&, requester] {
                iconRequesters_.remove(requester);
            });
        }

        auto key = cacheKey_(type, size, dpr);
        if (cache_.contains(key)) return cache_[key];

        auto path = registry_.value(type);
        if (path.isEmpty())
            FATAL("Path not set for icon [{}]", static_cast<int>(type));

        auto pixmap = renderSvg_(path, size, dpr);
        if (!pixmap.isNull()) cache_[key] = pixmap;
        return pixmap;
    }

    void trackAndStyle_(QMenuBar* menuBar) const
    {
        if (!menuBar) return;

        if (!menuStyleSheetRequesters_.contains(menuBar)) {
            menuStyleSheetRequesters_ << menuBar;
            connect(menuBar, &QObject::destroyed, this, [&, menuBar] {
                menuStyleSheetRequesters_.remove(menuBar);
            });
        }

        menuBar->setStyleSheet(menuStyleSheet_);
    }
};

} // namespace Fernanda
