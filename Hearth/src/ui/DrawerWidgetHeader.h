/*
 * Hearth — a plain-text-first workbench for creative writing
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
#include <QString>

#include "core/Debug.h"
#include "modules/StyleContext.h"
#include "ui/Icons.h"

namespace Hearth {

class DrawerWidgetHeader : public QAbstractButton
{
    Q_OBJECT

public:
    explicit DrawerWidgetHeader(QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        setup_();
    }

    virtual ~DrawerWidgetHeader() override { TRACER; }

    QSize iconSize() const noexcept { return iconSize_; }
    int spacing() const noexcept { return spacing_; }

    void setIconSize(const QSize& size)
    {
        if (iconSize_ == size) return;
        iconSize_ = size;
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

        return QSize(
            width + WIDTH_PADDING_,
            height + HEIGHT_PADDING_ + EXTRA_BOTTOM_PADDING_);
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

    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto widget_rect =
            contentsRect().adjusted(0, 0, 0, -EXTRA_BOTTOM_PADDING_);
        auto x = widget_rect.x();
        auto center_y = widget_rect.center().y();

        // Draw hover/press background
        // drawBackground_(painter, widget_rect);

        // Get icon from StyleContext
        auto icon = isChecked() ? UiIcon::ChevronDown : UiIcon::ChevronRight;
        auto pixmap = StyleContext::icon(this, icon, iconSize_);
        if (!pixmap.isNull()) {
            auto logical_size = pixmap.deviceIndependentSize();
            auto icon_y = center_y - logical_size.height() / 2;
            painter.drawPixmap(QPointF(x, icon_y), pixmap);
            x += logical_size.width() + spacing_;
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

private:
    static constexpr auto HIGHLIGHT_CORNER_RADIUS_ = 4;
    static constexpr auto HIGHLIGHT_INSET_ = 2;
    static constexpr auto WIDTH_PADDING_ = 8;
    static constexpr auto HEIGHT_PADDING_ = 6;
    static constexpr auto EXTRA_BOTTOM_PADDING_ = 4;

    QSize iconSize_{ 16, 16 };
    int spacing_ = 5;

    void setup_()
    {
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    // void drawBackground_(QPainter& painter, const QRect& rect) const
    // {
    /// TODO STYLE: Fix!

    // auto bg = Ui::resolveButtonColor(buttonColors_, isDown(),
    // underMouse()); if (!bg.isValid() || bg.alpha() <= 0) return;

    // QPainterPath path{};
    // auto highlight_rect = rect.adjusted(
    // HIGHLIGHT_INSET_,
    // HIGHLIGHT_INSET_,
    // -HIGHLIGHT_INSET_,
    // -HIGHLIGHT_INSET_);
    // path.addRoundedRect(
    // highlight_rect,
    // HIGHLIGHT_CORNER_RADIUS_,
    // HIGHLIGHT_CORNER_RADIUS_);

    // painter.fillPath(path, bg);
    // }
};

} // namespace Hearth
