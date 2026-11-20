/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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
#include <QEasingCurve>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>

#include "ui/Style.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

// A pill-shaped on/off switch, the control for every yes/no setting, as in
// Obsidian's settings. A checkable QAbstractButton, so checked(), toggled,
// Space, tab order, and accessibility are inherited; it only paints.
//
// The look follows Material 3: on, the track is the accent with a large thumb
// in the accent's text color at the right; off, the track is the window's
// color, outlined, with a smaller thumb in the outline's color at the left. A
// light thumb would vanish on an off track in a light theme, hence the
// difference.
//
// Every color is a palette role (UiConstants) in the current color group, so a
// disabled switch dims and a theme change restyles it. The accent is
// QPalette::Accent, which the Windows 11 style fills from the system's.
//
// One progress value, 0 to 1, animates a flip, and every part (thumb position
// and size, each color blended) is drawn from it. A change while hidden jumps
// to its end, so a page never opens mid-slide.
//
// A click doesn't take focus, as in Obsidian, so the focus ring, drawn in a
// margin around the pill, shows only for the keyboard
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit ToggleSwitch(QWidget* parent)
        : QAbstractButton(parent)
    {
        setup_();
    }

    ~ToggleSwitch() override { TRACER; }

    // The pill plus the focus ring's margin on every side, so the ring is
    // drawn inside the widget rather than clipped at its edge
    [[nodiscard]] QSize sizeHint() const override
    {
        return { TOGGLE_WIDTH + 2 * TOGGLE_FOCUS_MARGIN,
                 TOGGLE_HEIGHT + 2 * TOGGLE_FOCUS_MARGIN };
    }

    [[nodiscard]] QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Disabled: the same switch, faded. Its on/off state stays readable,
        // which the palette's Disabled colors don't promise for every role
        if (!isEnabled()) {
            painter.setOpacity(TOGGLE_DISABLED_OPACITY);
        }

        auto p = progress_;
        auto track = trackRect_();
        auto radius = track.height() / 2.0;
        auto colors = palette();

        // Track. The outline fades as the fill turns to accent, so the on
        // state reads as one solid pill
        auto fill = Style::blend(
            colors.color(TOGGLE_OFF_TRACK_ROLE),
            colors.color(TOGGLE_ON_TRACK_ROLE),
            p);
        auto outline = Style::blend(
            colors.color(TOGGLE_OFF_OUTLINE_ROLE),
            colors.color(TOGGLE_ON_TRACK_ROLE),
            p);

        // Inset by half the pen so the stroke stays inside the widget
        auto half_pen = TOGGLE_OUTLINE_WIDTH / 2.0;
        painter.setPen(QPen(outline, TOGGLE_OUTLINE_WIDTH));
        painter.setBrush(fill);
        painter.drawRoundedRect(
            track.adjusted(half_pen, half_pen, -half_pen, -half_pen),
            radius - half_pen,
            radius - half_pen);

        // Thumb: grows and slides along the track's centerline, between the
        // centers of the track's two round ends
        auto diameter =
            TOGGLE_OFF_THUMB_DIAMETER +
            (TOGGLE_ON_THUMB_DIAMETER - TOGGLE_OFF_THUMB_DIAMETER) * p;
        auto left_x = track.left() + radius;
        auto right_x = track.right() - radius;
        QPointF center(left_x + (right_x - left_x) * p, track.center().y());

        painter.setPen(Qt::NoPen);
        painter.setBrush(
            Style::blend(
                colors.color(TOGGLE_OFF_THUMB_ROLE),
                colors.color(TOGGLE_ON_THUMB_ROLE),
                p));
        painter.drawEllipse(center, diameter / 2.0, diameter / 2.0);

        // Our own focus ring, a pill outline in the margin around the track.
        // Not the style's PE_FrameFocusRect: the Windows 11 style draws that as
        // a filled box over the whole widget, which buries the switch
        if (hasFocus()) {
            auto ring_inset = TOGGLE_FOCUS_RING_WIDTH / 2.0;
            auto ring = QRectF(rect()).adjusted(
                ring_inset,
                ring_inset,
                -ring_inset,
                -ring_inset);
            auto ring_radius = ring.height() / 2.0;

            painter.setPen(QPen(
                colors.color(TOGGLE_FOCUS_RING_ROLE),
                TOGGLE_FOCUS_RING_WIDTH));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(ring, ring_radius, ring_radius);
        }
    }

private:
    // 0 = off, 1 = on; between the two only while animating
    qreal progress_ = 0.0;
    QVariantAnimation* animation_ = new QVariantAnimation(this);

    void setup_()
    {
        setCheckable(true);
        setFocusPolicy(Qt::TabFocus);
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        animation_->setDuration(TOGGLE_ANIMATION_MS);
        animation_->setEasingCurve(QEasingCurve::InOutCubic);

        connect(
            animation_,
            &QVariantAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                progress_ = value.toReal();
                update();
            });

        connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
            auto target = checked ? 1.0 : 0.0;
            animation_->stop();

            if (!isVisible()) {
                progress_ = target;
                update();
                return;
            }

            // From wherever the thumb is now, so flipping mid-slide reverses
            // smoothly instead of snapping to an end first
            animation_->setStartValue(progress_);
            animation_->setEndValue(target);
            animation_->start();
        });
    }

    // Centered in the widget at the pill's own size, inside the focus ring's
    // margin, so a layout that stretches the widget doesn't stretch the pill
    [[nodiscard]] QRectF trackRect_() const
    {
        auto w = qMin(TOGGLE_WIDTH, width());
        auto h = qMin(TOGGLE_HEIGHT, height());
        return QRectF((width() - w) / 2.0, (height() - h) / 2.0, w, h);
    }
};

} // namespace Fernanda::Ui
