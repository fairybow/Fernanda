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

#include "Debug.h"
#include "StyleContext.h"
#include "Ui.h"

// TODO: Verify this
namespace Fernanda {

// Header button for CollapsibleWidget with SVG icon support and text display.
// Shows different icons for expanded/collapsed states.
class CollapsibleWidgetHeader : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(
        QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor pressedColor READ pressedColor WRITE setPressedColor)

public:
    DECLARE_UI_BUTTON_COLOR_ACCESSORS(buttonColors_)

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
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto widget_rect = contentsRect();
        auto x = widget_rect.x();
        auto center_y = widget_rect.center().y();

        // Draw hover/press background
        drawBackground_(painter, widget_rect);

        // Get icon from StyleContext
        auto icon =
            isChecked() ? Ui::Icon::ChevronDown : Ui::Icon::ChevronRight;
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

    QSize iconSize_{ 16, 16 };
    int spacing_ = 5;

    Ui::ButtonColors buttonColors_{};

    void setup_()
    {
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void drawBackground_(QPainter& painter, const QRect& rect) const
    {
        auto bg = Ui::resolveButtonColor(buttonColors_, isDown(), underMouse());
        if (!bg.isValid() || bg.alpha() <= 0) return;

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

        painter.fillPath(path, bg);
    }
};

} // namespace Fernanda
