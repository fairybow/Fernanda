/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <cmath>
#include <numbers>

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QTextCursor>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// Transparent overlay that draws teardrop-shaped selection handles on top of a
// QPlainTextEdit viewport. Uses an event filter on the viewport for mouse input
// so that non-handle clicks pass through naturally.
//
// TODO: Hide handles after a delay when the mouse leaves the selection area
// (with a buffer zone to trigger reappearance)
// TODO: Make handle color stylable via Q_PROPERTY/QSS
// TODO: Settings toggle in editor panel!
class SelectionHandleOverlay : public QWidget
{
    Q_OBJECT

    // Future QSS styling:
    // Q_PROPERTY(QColor handleColor READ handleColor WRITE setHandleColor)
    // Q_PROPERTY(QColor handleBorderColor READ handleBorderColor WRITE
    //     setHandleBorderColor)

public:
    explicit SelectionHandleOverlay(QPlainTextEdit* editor)
        : QWidget(editor->viewport())
        , editor_(editor)
    {
        setup_();
    }

    virtual ~SelectionHandleOverlay() override { TRACER; }

    bool handlesEnabled() const { return handlesEnabled_; }
    void setHandlesEnabled(bool enabled) { handlesEnabled_ = enabled; }

protected:
    virtual void paintEvent(QPaintEvent* event) override
    {
        (void)event;

        if (!handlesEnabled_) return;
        if (!editor_->textCursor().hasSelection()) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        drawHandle_(painter, Handle_::Start);
        drawHandle_(painter, Handle_::End);
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        auto viewport = editor_->viewport();
        if (watched != viewport) return false;

        // Track viewport resizes so the overlay always covers the full area
        if (event->type() == QEvent::Resize) {
            setGeometry(viewport->rect());
            return false;
        }

        if (!handlesEnabled_) return false;
        if (!editor_->textCursor().hasSelection() && !isDragging_) return false;

        switch (event->type()) {
        case QEvent::MouseButtonPress:
            return handleMousePress_(static_cast<QMouseEvent*>(event));
        case QEvent::MouseMove:
            return handleMouseMove_(static_cast<QMouseEvent*>(event));
        case QEvent::MouseButtonRelease:
            return handleMouseRelease_(static_cast<QMouseEvent*>(event));
        default:
            return false;
        }
    }

private:
    enum class Handle_
    {
        None,
        Start,
        End
    };

    QPlainTextEdit* editor_;

    static constexpr qreal CIRCLE_RADIUS_ =
        7.0; // Radius of the teardrop's circular bulb
    static constexpr qreal STEM_HEIGHT_ =
        4.0; // Gap between the text baseline and the top of the circle
    static constexpr qreal HIT_RADIUS_ =
        14.0; // Clickable area radius (generous for easy targeting)

    bool handlesEnabled_ = true;
    bool isDragging_ = false;
    Handle_ activeHandle_ = Handle_::None;
    qreal dragYOffset_ = 0.0;
    int dragAnchorPosition_ = -1;

    QColor handleColor_{ Qt::white }; // TODO: Q_PROPERTY
    QColor handleBorderColor_{ 80, 80, 80, 180 }; // TODO: Q_PROPERTY

    void setup_()
    {
        // Mouse input is handled via eventFilter on the viewport, not on this
        // widget directly. This attribute ensures clicks pass through the
        // overlay to the viewport, where we intercept them
        setAttribute(Qt::WA_TransparentForMouseEvents);

        auto viewport = editor_->viewport();

        viewport->installEventFilter(this);
        // TODO: Is this fine to do? Will it mess any functionality up outside
        // this widget?
        viewport->setMouseTracking(true);

        setGeometry(viewport->rect());

        connect(editor_, &QPlainTextEdit::cursorPositionChanged, this, [&] {
            update();
        });

        connect(
            editor_,
            &QPlainTextEdit::updateRequest,
            this,
            [&](const QRect& rect, int deltaY) {
                (void)rect;
                // QWidget::scroll() on the viewport shifts child widgets by the
                // scroll delta. Reset our geometry so the overlay stays aligned
                if (deltaY) setGeometry(editor_->viewport()->rect());
                update();
            });
    }

    // Where the teardrop's tip touches the text (at the bottom of the cursor
    // rect for the given selection boundary)
    QPointF anchorPoint_(Handle_ handle) const
    {
        if (handle == Handle_::None) return {};

        auto cursor = editor_->textCursor();
        auto position = (handle == Handle_::Start) ? cursor.selectionStart()
                                                   : cursor.selectionEnd();
        cursor.setPosition(position);
        auto rect = editor_->cursorRect(cursor);

        return QPointF(rect.center().x(), rect.bottom());
    }

    // Center of the circular bulb (directly below the anchor)
    QPointF handleCenter_(Handle_ handle) const
    {
        auto anchor = anchorPoint_(handle);
        return { anchor.x(), anchor.y() + STEM_HEIGHT_ + CIRCLE_RADIUS_ };
    }

    qreal euclideanDistanceSquared_(
        const QPointF& point1,
        const QPointF& point2) const
    {
        QPointF adjusted(point2 - point1);
        auto x = adjusted.x();
        auto y = adjusted.y();
        return x * x + y * y;
    }

    Handle_ hitTest_(const QPoint& pos) const
    {
        if (!editor_->textCursor().hasSelection()) return Handle_::None;

        auto pos_f = QPointF(pos);
        auto hit_radius_sq = HIT_RADIUS_ * HIT_RADIUS_;

        // Check end first (if handles overlap, end is on top visually)
        if (euclideanDistanceSquared_(handleCenter_(Handle_::End), pos_f)
            <= hit_radius_sq)
            return Handle_::End;

        if (euclideanDistanceSquared_(handleCenter_(Handle_::Start), pos_f)
            <= hit_radius_sq)
            return Handle_::Start;

        return Handle_::None;
    }

    // Pointed tip at `anchor`, circular bulb below. The tip connects to the
    // circle via tangent lines, creating a smooth shape
    QPainterPath teardropPath_(const QPointF& anchor) const
    {
        auto dist = STEM_HEIGHT_ + CIRCLE_RADIUS_;
        QPointF center(anchor.x(), anchor.y() + dist);

        // Half-angle of the tangent lines from tip to circle
        auto alpha = std::acos(CIRCLE_RADIUS_ / dist);

        // Tangent points on the circle (in screen coordinates, y-down).
        // Direction from center to tip is straight up: (0, -dist).
        // Rotating that direction by +/-alpha gives the tangent directions.
        auto sin_a = std::sin(alpha);
        auto cos_a = std::cos(alpha);

        QPointF tangent_left(
            center.x() - CIRCLE_RADIUS_ * sin_a,
            center.y() - CIRCLE_RADIUS_ * cos_a);
        QPointF tangent_right(
            center.x() + CIRCLE_RADIUS_ * sin_a,
            center.y() - CIRCLE_RADIUS_ * cos_a);

        QPainterPath path{};
        path.moveTo(anchor);
        path.lineTo(tangent_left);

        // Arc from tangent_left around the bottom to tangent_right. Qt arc
        // angles: 0deg = 3 o'clock, positive = CCW in math (CW on screen).
        // tangent_left is at painting angle (90 + alpha)deg, tangent_right at
        // (90 - alpha)deg. Sweeping CCW (positive) by (360 - 2*alpha)deg goes
        // through the bottom
        QRectF circle_rect(
            center.x() - CIRCLE_RADIUS_,
            center.y() - CIRCLE_RADIUS_,
            CIRCLE_RADIUS_ * 2.0,
            CIRCLE_RADIUS_ * 2.0);

        auto alpha_deg = alpha * 180.0 / std::numbers::pi;
        auto start_angle = 90.0 + alpha_deg;
        auto sweep = 360.0 - 2.0 * alpha_deg;

        path.arcTo(circle_rect, start_angle, sweep);
        path.closeSubpath();

        return path;
    }

    void drawHandle_(QPainter& painter, Handle_ handle)
    {
        auto anchor = anchorPoint_(handle);
        // (Our rect is the viewport rect):
        if (!rect().contains(anchor.toPoint())) return;

        auto path = teardropPath_(anchor);
        painter.setPen(QPen(handleBorderColor_, 1.0));
        painter.setBrush(handleColor_);
        painter.drawPath(path);
    }

    bool handleMousePress_(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton) return false;

        auto pos = event->pos();
        auto handle = hitTest_(pos);
        if (handle == Handle_::None) return false;

        auto cursor = editor_->textCursor();

        isDragging_ = true;
        activeHandle_ = handle;
        dragYOffset_ = pos.y() - anchorPoint_(handle).y();
        dragAnchorPosition_ = (handle == Handle_::Start)
                                  ? cursor.selectionEnd()
                                  : cursor.selectionStart();

        editor_->viewport()->setCursor(Qt::ClosedHandCursor);

        return true; // Eat the event, don't deselect text
    }

    bool handleMouseMove_(QMouseEvent* event)
    {
        if (isDragging_) {
            updateDrag_(event->pos());
            return true;
        }

        // Cursor feedback on hover
        if (editor_->textCursor().hasSelection()) {
            auto handle = hitTest_(event->pos());
            editor_->viewport()->setCursor(
                handle != Handle_::None ? Qt::OpenHandCursor : Qt::IBeamCursor);
        }

        return false; // Let the viewport handle it normally
    }

    bool handleMouseRelease_(QMouseEvent* event)
    {
        (void)event;
        if (!isDragging_) return false;

        isDragging_ = false;
        activeHandle_ = Handle_::None;

        // Restore cursor based on current hover state
        auto handle = hitTest_(mapFromGlobal(QCursor::pos()));
        editor_->viewport()->setCursor(
            handle != Handle_::None ? Qt::OpenHandCursor : Qt::IBeamCursor);

        return true;
    }

    void updateDrag_(const QPoint& pos)
    {
        QPointF adjusted = QPoint(pos.x(), pos.y() - dragYOffset_);
        auto new_position =
            editor_->cursorForPosition(adjusted.toPoint()).position();

        QTextCursor cursor(editor_->textCursor());
        cursor.clearSelection();
        cursor.setPosition(dragAnchorPosition_);
        cursor.setPosition(new_position, QTextCursor::KeepAnchor);

        activeHandle_ = (new_position < dragAnchorPosition_) ? Handle_::Start
                                                             : Handle_::End;
        editor_->setTextCursor(cursor);
        update();
    }
};

} // namespace Fernanda
