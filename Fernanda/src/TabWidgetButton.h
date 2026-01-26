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

    std::optional<StyleContext::Icon> icon() const noexcept { return icon_; }

    void setIcon(StyleContext::Icon icon)
    {
        icon_ = icon;
        update();
    }

    std::optional<StyleContext::Icon> flagIcon() const noexcept
    {
        return flagIcon_;
    }

    void setFlagIcon(StyleContext::Icon icon)
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

        /// TODO STYLE: Fix!

        // Determine current background color
        //auto bg = Ui::resolveButtonColor(buttonColors_, isDown(), underMouse());

        // Draw background
        /*if (bg.isValid() && bg.alpha() > 0) {
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
        }*/

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

    std::optional<StyleContext::Icon> icon_{};
    std::optional<StyleContext::Icon> flagIcon_{};
    QSize iconSize_{ 16, 16 };
    bool flagged_ = false;

    void setup_()
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
    }

    std::optional<StyleContext::Icon> currentIcon_() const
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
