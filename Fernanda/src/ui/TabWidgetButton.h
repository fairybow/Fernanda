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

#include "core/Debug.h"
#include "modules/StyleContext.h"
#include "ui/Icons.h"

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

    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto widget_rect = rect();

        /// TODO STYLE: Fix!

        // Determine current background color
        // auto bg = Ui::resolveButtonColor(buttonColors_, isDown(),
        // underMouse());

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

    std::optional<UiIcon> icon_{};
    std::optional<UiIcon> flagIcon_{};
    QSize iconSize_{ 16, 16 };
    bool flagged_ = false;

    void setup_()
    {
        setAttribute(Qt::WA_StyledBackground, true); // For QSS
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
