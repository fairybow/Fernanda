/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include "ui/UiConstants.h"

namespace Suzuri::Ui {

// A pill-shaped on/off switch with a round thumb, the control every yes/no
// setting uses — Obsidian's settings use its toggle for every boolean, never a
// checkbox. A checkable QAbstractButton, so it is a real button to Qt:
// checked(), setChecked(), toggled(bool), Space to flip it, a place in the tab
// order, and accessibility, all inherited. It only paints.
//
// The look follows Material 3's switch:
//
//   On  — the track filled with the accent color; a large thumb in the
//         accent's text color (light) at the right end.
//   Off — the track in the window's own color, outlined; a smaller thumb in
//         the outline's color at the left end.
//
// The off state can't mirror the on one: a light thumb on a window-colored
// track would all but vanish in a light theme, so the off thumb takes the
// outline's tone and shrinks, and the outline gives the track its shape.
//
// Every color is a palette role (UiConstants), resolved in the widget's
// current color group like everything Qt paints, so a disabled switch dims and
// a theme change restyles it with no bookkeeping here. The accent is
// QPalette::Accent (Qt 6.6+), which the Windows 11 style fills from the
// system accent.
//
// Flipping animates: one progress value runs 0 → 1 (off → on) and every part
// — thumb position, thumb size, and the colors, each blended between its two
// states — is drawn from it, so there is exactly one animated number. A
// change while hidden (the initial setChecked, before the dialog shows) jumps
// straight to its end, so a page never opens mid-slide.
//
// Tab focus only: a click doesn't take focus, as Obsidian's toggle doesn't,
// so the focus ring appears only for someone moving by keyboard. The ring is
// drawn here, in a margin kept around the pill for it
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
        auto fill = blend_(
            colors.color(TOGGLE_OFF_TRACK_ROLE),
            colors.color(TOGGLE_ON_TRACK_ROLE),
            p);
        auto outline = blend_(
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
        painter.setBrush(blend_(
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
        auto w = qMin(qreal(TOGGLE_WIDTH), qreal(width()));
        auto h = qMin(qreal(TOGGLE_HEIGHT), qreal(height()));
        return QRectF((width() - w) / 2.0, (height() - h) / 2.0, w, h);
    }

    [[nodiscard]] static QColor
    blend_(const QColor& from, const QColor& to, qreal t)
    {
        auto mix = [t](qreal a, qreal b) { return a + (b - a) * t; };
        return QColor::fromRgbF(
            float(mix(from.redF(), to.redF())),
            float(mix(from.greenF(), to.greenF())),
            float(mix(from.blueF(), to.blueF())),
            float(mix(from.alphaF(), to.alphaF())));
    }
};

} // namespace Suzuri::Ui
