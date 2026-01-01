/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QHash>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QSvgRenderer>

#include "Debug.h"

// TODO: Verify this
// TODO: Combine any common SVG code from TabWidgetButton into namespace
namespace Fernanda {

// Header button for CollapsibleWidget with SVG icon support and text display.
// Shows different icons for expanded/collapsed states.
class CollapsibleWidgetHeader : public QAbstractButton
{
    Q_OBJECT

public:
    explicit CollapsibleWidgetHeader(QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        setup_();
    }

    virtual ~CollapsibleWidgetHeader() override { TRACER; }

    QSize iconSize() const noexcept { return iconSize_; }
    int spacing() const noexcept { return spacing_; }

    void setIconSize(const QSize& size)
    {
        if (iconSize_ == size) return;
        iconSize_ = size;
        invalidateCache_();
        update();
    }

    void setSpacing(int spacing)
    {
        if (spacing_ == spacing) return;
        spacing_ = spacing;
        update();
    }

    virtual QSize sizeHint() const override
    {
        auto fm = fontMetrics();
        auto text_width = fm.horizontalAdvance(text());
        auto text_height = fm.height();

        auto width = iconSize_.width() + spacing_ + text_width;
        auto height = qMax(iconSize_.height(), text_height);

        // Add some padding
        return QSize(width + 8, height + 8);
    }

    virtual QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    virtual void enterEvent(QEnterEvent* event) override
    {
        QAbstractButton::enterEvent(event);
        update();
    }

    virtual void leaveEvent(QEvent* event) override
    {
        QAbstractButton::leaveEvent(event);
        update();
    }

    virtual void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        setPainterProperties_(painter);

        auto widget_rect = contentsRect();
        auto x = widget_rect.x();
        auto center_y = widget_rect.center().y();

        // Draw hover/press background
        drawBackground_(painter, widget_rect);

        // Draw icon
        QString svg_path = isChecked() ? expandedSvgPath_ : collapsedSvgPath_;
        if (!svg_path.isEmpty()) {
            auto pixmap = getCachedPixmap_(svg_path);
            if (!pixmap.isNull()) {
                auto logical_size = pixmap.deviceIndependentSize();
                auto icon_y = center_y - logical_size.height() / 2;
                painter.drawPixmap(QPointF(x, icon_y), pixmap);
                x += logical_size.width() + spacing_;
            }
        }

        // Draw text
        auto text_rect = QRect(
            x,
            widget_rect.y(),
            widget_rect.right() - x,
            widget_rect.height());
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, text());
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        QAbstractButton::resizeEvent(event);
        invalidateCache_();
    }

private:
    static constexpr auto HIGHLIGHT_CORNER_RADIUS_ = 4;
    static constexpr auto HIGHLIGHT_INSET_ = 2;
    static constexpr auto CACHE_FORMAT_ = "%1_%2x%3";

    QString expandedSvgPath_ = ":/ui/ChevronDown.svg";
    QString collapsedSvgPath_ = ":/ui/ChevronRight.svg";
    QSize iconSize_{ 16, 16 };
    int spacing_ = 5;
    mutable QHash<QString, QPixmap> pixmapCache_{};

    void setup_()
    {
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setPainterProperties_(QPainter& painter) const
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    void drawBackground_(QPainter& painter, const QRect& rect) const
    {
        auto pressed = isDown();
        auto under_mouse = underMouse();

        if (!pressed && !under_mouse) return;

        QPainterPath path{};
        auto highlight_rect = rect.adjusted(
            HIGHLIGHT_INSET_,
            HIGHLIGHT_INSET_,
            -HIGHLIGHT_INSET_,
            -HIGHLIGHT_INSET_);
        path.addRoundedRect(
            highlight_rect,
            HIGHLIGHT_CORNER_RADIUS_,
            HIGHLIGHT_CORNER_RADIUS_);

        if (pressed)
            painter.fillPath(path, QColor(0, 0, 0, 30));
        else if (under_mouse)
            painter.fillPath(path, QColor(0, 0, 0, 15));
    }

    void invalidateCache_() { pixmapCache_.clear(); }

    QPixmap getCachedPixmap_(const QString& svgPath) const
    {
        if (svgPath.isEmpty()) return {};

        auto cache_key = QString(CACHE_FORMAT_)
                             .arg(svgPath)
                             .arg(iconSize_.width())
                             .arg(iconSize_.height());

        if (pixmapCache_.contains(cache_key)) return pixmapCache_[cache_key];

        auto pixmap = renderSvgToPixmap_(svgPath, iconSize_);
        if (!pixmap.isNull()) pixmapCache_[cache_key] = pixmap;
        return pixmap;
    }

    QPixmap
    renderSvgToPixmap_(const QString& svgPath, const QSize& targetSize) const
    {
        QSvgRenderer renderer(svgPath);

        if (!renderer.isValid()) {
            WARN("Failed to load SVG [{}]!", svgPath);
            return {};
        }

        auto device_pixel_ratio = devicePixelRatio();
        auto pixmap_size = targetSize * device_pixel_ratio;

        QPixmap pixmap(pixmap_size);
        pixmap.setDevicePixelRatio(device_pixel_ratio);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        setPainterProperties_(painter);

        renderer.render(&painter, QRectF({ 0, 0 }, QSizeF(targetSize)));

        return pixmap;
    }
};

} // namespace Fernanda
