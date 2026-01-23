/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <optional>

#include <QAbstractButton>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QSizeF>

#include "Debug.h"
#include "StyleContext.h"

namespace Fernanda {

// Tab button widget with SVG icon support, hover states, and optional flagged
// state
class TabWidgetButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(
        QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor pressedColor READ pressedColor WRITE setPressedColor)

public:
    explicit TabWidgetButton(QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        setup_();
    }

    virtual ~TabWidgetButton() override { TRACER; }

    bool flagged() const noexcept { return flagged_; }

    void setFlagged(bool flagged)
    {
        if (flagged_ == flagged) return;
        flagged_ = flagged;
        update();
    }

    std::optional<UiIcon> icon() const noexcept { return icon_; }

    void setIcon(UiIcon icon)
    {
        icon_ = icon;
        update();
    }

    std::optional<UiIcon> flagIcon() const noexcept { return flagIcon_; }

    void setFlagIcon(UiIcon icon)
    {
        flagIcon_ = icon;
        update();
    }

    QSize iconSize() const noexcept { return iconSize_; }

    void setIconSize(const QSize& size)
    {
        if (iconSize_ == size) return;
        iconSize_ = size;
        update();
    }

    QColor backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const QColor& color)
    {
        if (backgroundColor_ == color) return;
        backgroundColor_ = color;
        update();
    }

    QColor hoverColor() const { return hoverColor_; }
    void setHoverColor(const QColor& color)
    {
        if (hoverColor_ == color) return;
        hoverColor_ = color;
        update();
    }

    QColor pressedColor() const { return pressedColor_; }
    void setPressedColor(const QColor& color)
    {
        if (pressedColor_ == color) return;
        pressedColor_ = color;
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
        (void)event;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto widget_rect = rect();

        // Determine current background color
        QColor bg = backgroundColor_;
        if (isDown() && pressedColor_.isValid()) {
            bg = pressedColor_;
        } else if (underMouse() && hoverColor_.isValid()) {
            bg = hoverColor_;
        }

        // Draw background
        if (bg.isValid() && bg.alpha() > 0) {
            QPainterPath path;
            auto bg_rect = widget_rect.adjusted(
                HIGHLIGHT_INSET_,
                HIGHLIGHT_INSET_,
                -HIGHLIGHT_INSET_,
                -HIGHLIGHT_INSET_);
            path.addRoundedRect(
                bg_rect,
                HIGHLIGHT_CORNER_RADIUS_,
                HIGHLIGHT_CORNER_RADIUS_);
            painter.fillPath(path, bg);
        }

        // Get icon from ProxyStyle
        auto icon = currentIcon_();
        if (!icon) return;

        auto pixmap = StyleContext::icon(this, *icon, effectiveIconSize_());
        if (pixmap.isNull()) return;

        drawCenteredPixmap_(painter, pixmap, widget_rect);
    }

private:
    static constexpr auto HIGHLIGHT_CORNER_RADIUS_ = 4;
    static constexpr auto HIGHLIGHT_INSET_ = 2;

    std::optional<UiIcon> icon_{};
    std::optional<UiIcon> flagIcon_{};
    QSize iconSize_{ 16, 16 };
    bool flagged_ = false;

    QColor backgroundColor_{ Qt::transparent };
    QColor hoverColor_{ Qt::transparent };
    QColor pressedColor_{ Qt::transparent };

    void setup_()
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
    }

    std::optional<UiIcon> currentIcon_() const
    {
        if (flagged_ && flagIcon_ && !underMouse()) return flagIcon_;
        return icon_;
    }

    QSize effectiveIconSize_() const
    {
        auto size = contentsRect().size();
        return { qMin(iconSize_.width(), size.width()),
                 qMin(iconSize_.height(), size.height()) };
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
