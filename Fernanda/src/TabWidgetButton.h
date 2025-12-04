/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

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
#include <QToolButton>
#include <QVariant>

#include "Debug.h"

namespace Fernanda {

// Tab button widget with SVG icon support, hover states, and optional flagged
// state
class TabWidgetButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit TabWidgetButton(QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        setup_();
    }

    virtual ~TabWidgetButton() override { TRACER; }

    // Idk about this
    QString text() const {}
    void setText(const QString& text) {}

    bool flagged() const noexcept { return flagged_; }
    QString svgPath() const noexcept { return svgPath_; }
    QString flagSvgPath() const noexcept { return flagSvgPath_; }
    QSize svgSize() const noexcept { return svgSize_; }

    void setFlagged(bool flagged)
    {
        if (flagged_ == flagged) return;
        flagged_ = flagged;
        update();
    }

    void setSvgPath(const QString& svgPath)
    {
        if (svgPath_ == svgPath) return;
        svgPath_ = svgPath;
        invalidateCache_();
        update();
    }

    void setFlagSvgPath(const QString& flagSvgPath)
    {
        if (flagSvgPath_ == flagSvgPath) return;
        flagSvgPath_ = flagSvgPath;
        invalidateCache_();
        update();
    }

    void setSvgSize(const QSize& size)
    {
        if (svgSize_ == size) return;
        svgSize_ = size;
        invalidateCache_();
        update();
    }

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

        auto widget_rect = rect();

        // Draw background based on button state
        auto pressed = isDown();
        auto under_mouse = underMouse();

        // Transparent background for normal state
        if (pressed || under_mouse) {
            QPainterPath path{};

            // Shrink and round out the highlight
            auto highlight_rect = widget_rect.adjusted(
                HIGHLIGHT_INSET_,
                HIGHLIGHT_INSET_,
                -HIGHLIGHT_INSET_,
                -HIGHLIGHT_INSET_);
            path.addRoundedRect(
                highlight_rect,
                HIGHLIGHT_CORNER_RADIUS_,
                HIGHLIGHT_CORNER_RADIUS_);

            // Fill
            if (pressed)
                painter.fillPath(path, QColor(0, 0, 0, 30));
            else if (under_mouse)
                painter.fillPath(path, QColor(0, 0, 0, 15));
        }

        QString svg_path = shouldShowFlag_() ? flagSvgPath_ : svgPath_;
        if (svg_path.isEmpty()) return;

        auto pixmap = getCachedPixmap_(svg_path);
        if (pixmap.isNull()) return;

        drawCenteredPixmap_(painter, pixmap, widget_rect);
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        QAbstractButton::resizeEvent(event);
        invalidateCache_();
    }

private:
    static constexpr auto FLAG_PROPERTY_ = "flagged";
    static constexpr auto HIGHLIGHT_CORNER_RADIUS_ = 4;
    static constexpr auto HIGHLIGHT_INSET_ = 2;
    static constexpr auto CACHE_FORMAT_ = "%1_%2x%3";

    QString svgPath_{};
    QString flagSvgPath_{};
    QSize svgSize_{ 16, 16 };
    bool flagged_ = false;
    mutable QHash<QString, QPixmap> pixmapCache_{};

    void setup_()
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
    }

    bool shouldShowFlag_() const
    {
        return flagged_ && !flagSvgPath_.isEmpty() && !underMouse();
    }

    void setPainterProperties_(QPainter& painter) const
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    void invalidateCache_() { pixmapCache_.clear(); }

    QPixmap getCachedPixmap_(const QString& svgPath) const
    {
        if (svgPath.isEmpty()) return {};

        // Key includes effective size
        auto target_size = getEffectiveIconSize_();
        auto cache_key = QString(CACHE_FORMAT_)
                             .arg(svgPath)
                             .arg(target_size.width())
                             .arg(target_size.height());

        if (pixmapCache_.contains(cache_key)) return pixmapCache_[cache_key];

        auto pixmap = renderSvgToPixmap_(svgPath, target_size);
        if (!pixmap.isNull()) pixmapCache_[cache_key] = pixmap;
        return pixmap;
    }

    QPixmap
    renderSvgToPixmap_(const QString& svgPath, const QSize& targetSize) const
    {
        QSvgRenderer renderer(svgPath);

        if (!renderer.isValid()) {
            qWarning() << "Failed to load SVG:" << svgPath;
            return {};
        }

        auto device_pixel_ratio = devicePixelRatio();
        auto pixmap_size = targetSize * device_pixel_ratio;

        QPixmap pixmap(pixmap_size);
        pixmap.setDevicePixelRatio(device_pixel_ratio);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        setPainterProperties_(painter);

        // Render to the LOGICAL bounds (painter coordinates are logical when
        // the pixmap has devicePixelRatio set)
        renderer.render(&painter, QRectF({ 0, 0 }, { targetSize }));

        return pixmap;
    }

    QSize getEffectiveIconSize_() const
    {
        auto size = contentsRect().size();

        return { qMin(svgSize_.width(), size.width()),
                 qMin(svgSize_.height(), size.height()) };
    }

    void drawCenteredPixmap_(
        QPainter& painter,
        const QPixmap& pixmap,
        const QRect& widgetRect) const
    {
        // Paint centered (use logical size for centering calculations)
        auto logical_size = pixmap.deviceIndependentSize();
        auto x =
            widgetRect.x() + (widgetRect.width() - logical_size.width()) / 2;
        auto y =
            widgetRect.y() + (widgetRect.height() - logical_size.height()) / 2;
        painter.drawPixmap(QPointF(x, y), pixmap);
    }
};

} // namespace Fernanda
