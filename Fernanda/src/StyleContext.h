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
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
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

class StyleContext : public QObject
{
    Q_OBJECT

public:
    static constexpr auto PROPERTY_KEY = "_styleContext";

    enum class Icon
    {
        ChevronDown,
        ChevronLeft,
        ChevronRight,
        ChevronUp,
        Dot,
        Plus,
        X
    };

    explicit StyleContext(QObject* parent = nullptr)
        : QObject(parent)
    {
        setup_();
    }

    virtual ~StyleContext() override { TRACER; }

    // Static API for widgets

    static QPixmap icon(QWidget* widget, Icon type, const QSize& size)
    {
        auto context = forWidget_(widget);
        if (!context) return {};

        return context->icon_(type, size, widget->devicePixelRatio());
    }

    // TODO: Any other colors for widgets/things we want to be style-aware that
    // can't accept QSS (block cursor color, perhaps)

    // StyleModule API

    // TODO: Clearer name?
    void attach(QWidget* window)
    {
        if (!window) return;

        trackRequester_(window);
        window->setProperty(PROPERTY_KEY, QVariant::fromValue(this));
    }

    static QColor defaultIconColor() { return DEFAULT_ICON_COLOR_; }

    void setIconColor(const QColor& color)
    {
        if (iconColor_ == color) return;
        iconColor_ = color;
        iconCache_.clear();
        updateRequesters_();
    }

    // Other setters...

private:
    static constexpr auto SVG_PLACEHOLDER_COLOR_ = "#404040";

    static constexpr auto DEFAULT_ICON_COLOR_ = SVG_PLACEHOLDER_COLOR_;
    static constexpr qreal SUBTLE_HOVER_STRENGTH = 0.04; // 10 alpha
    static constexpr qreal SUBTLE_PRESSED_STRENGTH = 0.08; // 20 alpha
    static constexpr qreal STRONG_HOVER_STRENGTH = 0.12; // 30 alpha
    static constexpr qreal STRONG_PRESSED_STRENGTH = 0.16; // 40 alpha


    QColor iconColor_{}; // TODO: Compute instead of having valid contructed color?
    QColor subtleHover_{};
    QColor subtlePressed_{};
    QColor strongHover_{};
    QColor strongPressed_{};

    QHash<Icon, QString> iconRegistry_{};
    mutable QHash<QString, QPixmap> iconCache_{};
    //...

    mutable QSet<QWidget*> requesters_{};

    void setup_()
    {
        iconRegistry_[Icon::ChevronDown] = ":/ui/ChevronDown.svg";
        iconRegistry_[Icon::ChevronLeft] = ":/ui/ChevronLeft.svg";
        iconRegistry_[Icon::ChevronRight] = ":/ui/ChevronRight.svg";
        iconRegistry_[Icon::ChevronUp] = ":/ui/ChevronUp.svg";
        iconRegistry_[Icon::Dot] = ":/ui/Dot.svg";
        iconRegistry_[Icon::Plus] = ":/ui/Plus.svg";
        iconRegistry_[Icon::X] = ":/ui/X.svg";
    }

    // TODO: Better/clearer name?
    static StyleContext* forWidget_(QWidget* widget)
    {
        if (!widget) return nullptr;
        auto window = widget->window();
        if (!window) return nullptr;

        return window->property(PROPERTY_KEY).value<StyleContext*>();
    }

    void trackRequester_(QWidget* widget) const
    {
        if (!widget || requesters_.contains(widget)) return;

        requesters_ << widget;
        connect(widget, &QObject::destroyed, this, [&, widget] {
            requesters_.remove(widget);
        });
    }

    void updateRequesters_()
    {
        for (auto& requester : requesters_)
            if (requester) requester->update();
    }

    QString iconCacheKey_(Icon type, const QSize& size, qreal dpr) const
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
        data.replace(SVG_PLACEHOLDER_COLOR_, color_hex);

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

    QPixmap icon_(Icon type, const QSize& size, qreal dpr) const
    {
        if (!size.isValid()) return {};

        auto key = iconCacheKey_(type, size, dpr);
        if (iconCache_.contains(key)) return iconCache_[key];

        auto path = iconRegistry_.value(type);
        if (path.isEmpty()) {
            FATAL("Path not set for icon [{}]", static_cast<int>(type));
        }

        auto pixmap = renderSvg_(path, size, dpr);
        if (!pixmap.isNull()) iconCache_[key] = pixmap;
        return pixmap;
    }
};

} // namespace Fernanda
