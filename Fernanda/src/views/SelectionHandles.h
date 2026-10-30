/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <cmath>
#include <numbers>
#include <optional>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QTextCursor>
#include <QWidget>

#include "views/ViewConstants.h"

namespace Fernanda {

// The two teardrop handles that hang under the ends of a text selection and can
// be dragged to move them.
//
// Not a widget: the editor paints the handles into its own viewport
// (TextEditor::paintEvent, after the text) and offers it each viewport mouse
// event first. So the handles scroll with the text they hang from; a
// transparent overlay widget would have to be kept aligned with the viewport as
// it scrolls and resizes.
//
// The price of painting into the viewport is that Qt repaints only the rows
// whose selection changed, and a handle hangs below its row. paint() covers
// that itself: it remembers where it last drew, and when the handles have
// since moved (or gone) it schedules a repaint of the old and new places.
// Every way a handle can move — the selection changing, the view scrolling,
// the text reflowing — repaints some of the viewport, so paint() always runs
// to notice.
//
// The handles are for adjusting a selection already made, so they stay hidden
// while one is being made with the mouse: from a left press (or double-click)
// that isn't on a handle until its release. A release can go missing — focus
// lost mid-drag, or a drag of the selected text, whose release Qt's drag
// consumes — so a mouse move with the button up ends it too. A selection made
// from the keyboard shows its handles as it grows.
//
// A plain class the editor owns by value, using only QPlainTextEdit's public
// interface. Colors are palette roles read at each paint, so they follow
// palette and color-group changes. Left-to-right text only
class SelectionHandles
{
public:
    explicit SelectionHandles(QPlainTextEdit* parentEditor)
        : editor_(parentEditor)
    {
    }

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    // The viewport tracks the mouse only while the handles are on: the hover
    // cursor needs move events with no button held, and nothing else here
    // does
    void setEnabled(bool enabled)
    {
        if (enabled == enabled_) {
            return;
        }

        enabled_ = enabled;
        dragging_ = false;
        selecting_ = false;
        setHovering_(false);

        auto* viewport = editor_->viewport();
        viewport->setMouseTracking(enabled);
        viewport->update();
    }

    // Draw the handles, from the editor's paintEvent once the text is drawn.
    // Qt has already clipped painting to the region being repainted
    void paint()
    {
        auto* viewport = editor_->viewport();

        auto start = anchor_(Handle_::Start);
        auto end = anchor_(Handle_::End);
        auto bounds = bounds_(start, end);

        // Moved since the last paint: repaint where they were and where
        // they are. That paint finds them where this one left them, and
        // schedules nothing
        if (bounds != paintedBounds_) {
            viewport->update(paintedBounds_ + bounds);
            paintedBounds_ = bounds;
        }

        if (bounds.isEmpty()) {
            return;
        }

        const auto& palette = editor_->palette();

        QPainter painter(viewport);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(
            palette.color(SELECTION_HANDLE_BORDER_ROLE),
            SELECTION_HANDLE_BORDER_WIDTH));
        painter.setBrush(palette.brush(SELECTION_HANDLE_FILL_ROLE));

        if (start) {
            painter.drawPath(teardropPath_(*start));
        }

        if (end) {
            painter.drawPath(teardropPath_(*end));
        }
    }

    // The editor offers each viewport mouse event here first; true means it
    // was taken, and the editor must not handle it too.
    //
    // A press on a handle starts dragging it. The selection's other end
    // stays put for the whole drag.
    //
    // The drag is measured from the middle of the row the handle hangs from,
    // not from its tip: the tip sits on the row's bottom edge, where one
    // pixel of downward movement is already the next row. From the middle,
    // the mouse has to travel half a row either way before the end changes
    // rows.
    //
    // A press anywhere else starts a selection, and hides the handles until
    // it is made
    bool mousePress(const QMouseEvent* event)
    {
        if (!enabled_ || event->button() != Qt::LeftButton) {
            return false;
        }

        auto position = event->position();
        auto handle = hitTest_(position);

        if (handle == Handle_::None) {
            setSelecting_(true);
            return false;
        }

        auto cursor = editor_->textCursor();

        dragging_ = true;
        dragOffset_ = position - QPointF(caretRect_(handle)->center());
        dragFixedPosition_ = (handle == Handle_::Start)
                                 ? cursor.selectionEnd()
                                 : cursor.selectionStart();

        editor_->viewport()->setCursor(Qt::ClosedHandCursor);
        return true;
    }

    // While dragging, the selection runs from the fixed end to the text at
    // the mouse, less the offset the press measured: the point that started
    // in the middle of the handle's row. Dragging past the fixed end simply
    // selects the other way.
    //
    // Not dragging, only the hover cursor changes, and the move is left to
    // the editor. A move with the button up ends a selection whose release
    // never arrived
    bool mouseMove(const QMouseEvent* event)
    {
        if (!enabled_) {
            return false;
        }

        if (!dragging_) {
            if (!(event->buttons() & Qt::LeftButton)) {
                setSelecting_(false);
            }

            setHovering_(hitTest_(event->position()) != Handle_::None);
            return false;
        }

        auto target = (event->position() - dragOffset_).toPoint();
        auto dragged_position = editor_->cursorForPosition(target).position();

        auto cursor = editor_->textCursor();
        cursor.setPosition(dragFixedPosition_);
        cursor.setPosition(dragged_position, QTextCursor::KeepAnchor);
        editor_->setTextCursor(cursor);

        return true;
    }

    // The end of a selection made with the mouse shows the handles. The
    // release itself is the editor's
    bool mouseRelease(const QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton) {
            return false;
        }

        if (!dragging_) {
            setSelecting_(false);
            return false;
        }

        dragging_ = false;

        // Whatever cursor the drag left, set the one for where it ended
        hovering_ = hitTest_(event->position()) != Handle_::None;
        editor_->viewport()->setCursor(
            hovering_ ? Qt::OpenHandCursor : Qt::IBeamCursor);

        return true;
    }

    // A double-click selects a word, and its button may be held a while
    // before the release, so it hides the handles as a press does. Never
    // taken: the editor selects the word
    void mouseDoubleClick(const QMouseEvent* event)
    {
        if (enabled_ && event->button() == Qt::LeftButton) {
            setSelecting_(true);
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

    bool enabled_ = false;

    // Where the handles were last drawn, in viewport coordinates (see paint)
    QRegion paintedBounds_{};

    bool hovering_ = false;
    bool dragging_ = false;

    // A selection is being made with the mouse (see class note)
    bool selecting_ = false;

    QPointF dragOffset_{};
    int dragFixedPosition_ = 0;

    // The text cursor's rectangle at one end of the selection, in viewport
    // coordinates: as tall as that end's row. Nothing when the handles are
    // off or hidden while a selection is made, when there is no selection,
    // or when that row is scrolled out of view
    [[nodiscard]] std::optional<QRect> caretRect_(Handle_ handle) const
    {
        if (!enabled_ || selecting_ || handle == Handle_::None) {
            return std::nullopt;
        }

        auto cursor = editor_->textCursor();

        if (!cursor.hasSelection()) {
            return std::nullopt;
        }

        cursor.setPosition(
            (handle == Handle_::Start) ? cursor.selectionStart()
                                       : cursor.selectionEnd());

        auto rect = editor_->cursorRect(cursor);

        if (rect.isEmpty() || !editor_->viewport()->rect().intersects(rect)) {
            return std::nullopt;
        }

        return rect;
    }

    // Where a handle's tip touches the text: the bottom of that rectangle
    [[nodiscard]] std::optional<QPointF> anchor_(Handle_ handle) const
    {
        auto rect = caretRect_(handle);

        if (!rect) {
            return std::nullopt;
        }

        return QPointF(rect->center().x(), rect->bottom());
    }

    // The center of the round bulb, straight below the tip
    [[nodiscard]] static QPointF bulbCenter_(const QPointF& anchor)
    {
        return { anchor.x(),
                 anchor.y() + SELECTION_HANDLE_STEM_HEIGHT +
                     SELECTION_HANDLE_RADIUS };
    }

    // What a handle covers when drawn, with room for its border and
    // antialiasing
    [[nodiscard]] static QRect bounds_(const QPointF& anchor)
    {
        constexpr qreal slack = 2.0;

        QRectF drawn(
            anchor.x() - SELECTION_HANDLE_RADIUS,
            anchor.y(),
            SELECTION_HANDLE_RADIUS * 2.0,
            SELECTION_HANDLE_STEM_HEIGHT + SELECTION_HANDLE_RADIUS * 2.0);

        return drawn.adjusted(-slack, -slack, slack, slack).toAlignedRect();
    }

    // What both handles cover, where each is drawn
    [[nodiscard]] static QRegion bounds_(
        const std::optional<QPointF>& start,
        const std::optional<QPointF>& end)
    {
        QRegion bounds{};

        if (start) {
            bounds += bounds_(*start);
        }

        if (end) {
            bounds += bounds_(*end);
        }

        return bounds;
    }

    // Hide or show the handles for a selection made with the mouse, and
    // repaint where they were and where they now go. Neither need come with
    // a repaint of its own: a release changes no text, and a press inside
    // the selection (the start of a drag of it) changes nothing yet
    void setSelecting_(bool selecting)
    {
        if (selecting == selecting_) {
            return;
        }

        auto before = bounds_(anchor_(Handle_::Start), anchor_(Handle_::End));
        selecting_ = selecting;
        auto after = bounds_(anchor_(Handle_::Start), anchor_(Handle_::End));

        editor_->viewport()->update(before + after);
    }

    // Which handle a viewport position is on: within HIT_RADIUS of a bulb's
    // center, a target larger than the bulb. The end handle is tested first,
    // so it wins where the two overlap
    [[nodiscard]] Handle_ hitTest_(const QPointF& position) const
    {
        auto hits = [&position](const std::optional<QPointF>& anchor) {
            if (!anchor) {
                return false;
            }

            auto offset = position - bulbCenter_(*anchor);

            return QPointF::dotProduct(offset, offset) <=
                   SELECTION_HANDLE_HIT_RADIUS * SELECTION_HANDLE_HIT_RADIUS;
        };

        if (hits(anchor_(Handle_::End))) {
            return Handle_::End;
        }

        if (hits(anchor_(Handle_::Start))) {
            return Handle_::Start;
        }

        return Handle_::None;
    }

    // An open hand over a handle, the text area's I-beam elsewhere. Set only
    // when it changes, not on every mouse move
    void setHovering_(bool hovering)
    {
        if (hovering == hovering_) {
            return;
        }

        hovering_ = hovering;
        editor_->viewport()->setCursor(
            hovering ? Qt::OpenHandCursor : Qt::IBeamCursor);
    }

    // A teardrop: the tip at anchor, the bulb below it, joined by the two
    // lines from the tip that are tangent to the bulb
    [[nodiscard]] static QPainterPath teardropPath_(const QPointF& anchor)
    {
        constexpr qreal radius = SELECTION_HANDLE_RADIUS;

        auto center = bulbCenter_(anchor);
        auto tip_distance = SELECTION_HANDLE_STEM_HEIGHT + radius;

        // The angle, at the bulb's center, between the line up to the tip
        // and the line to either tangent point
        auto alpha = std::acos(radius / tip_distance);

        QPointF left_tangent(
            center.x() - radius * std::sin(alpha),
            center.y() - radius * std::cos(alpha));

        QRectF bulb(
            center.x() - radius,
            center.y() - radius,
            radius * 2.0,
            radius * 2.0);

        // arcTo's angles are in degrees, counter-clockwise from 3 o'clock:
        // the left tangent point is at 90 + alpha, and sweeping round the
        // bottom to the right tangent point is a full turn less 2 * alpha.
        // closeSubpath then draws the line from there back to the tip
        auto alpha_degrees = alpha * 180.0 / std::numbers::pi;

        QPainterPath path{};
        path.moveTo(anchor);
        path.lineTo(left_tangent);
        path.arcTo(bulb, 90.0 + alpha_degrees, 360.0 - 2.0 * alpha_degrees);
        path.closeSubpath();

        return path;
    }
};

} // namespace Fernanda
