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

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QScrollBar>
#include <QWidget>
#include <QWindow>

#include <Coco/Time.h>

// Keeps hover washes in step with the pointer when widgets move under a
// pointer that stays put, and an item view's hovered row in step while one of
// its scroll bars is dragged. See docs/Architecture.md, "The style".
//
// Qt sets Qt::WA_UnderMouse, which a wash is drawn from, only when the mouse
// moves or a widget is shown or hidden. A widget that moves or resizes under a
// still pointer keeps its old mark until the next move, and one disabled as
// the pointer passes gets none. sync brings that move forward, handing the
// window a mouse move at the pointer so Qt sends the Enter and Leave a real
// move would. Setting the flag by hand would leave Qt's own record behind, and
// the widget would never get the Leave that clears it.
//
// An item view hovers a row only when its viewport gets a hover event, and
// syncView and clearView send the one a real move would. While a scroll bar
// is dragged it has the mouse, so the viewport gets none; followScrollBars
// sends them, so no row is left hovered when the pointer moves off without a
// scroll, or is let go outside the view
namespace Fernanda::Ui::HoverSync {

// A mouse move at the pointer, for widget's window, if the pointer is over
// that window and no button is held (a drag is under way otherwise)
inline void sync(QWidget* widget)
{
    auto* window = widget->window();
    auto* handle = window ? window->windowHandle() : nullptr;

    if (!handle || !window->isVisible() ||
        QGuiApplication::mouseButtons() != Qt::NoButton) {
        return;
    }

    auto global = QCursor::pos();
    auto local = handle->mapFromGlobal(global);

    if (!QRect(QPoint(0, 0), handle->size()).contains(local)) {
        return;
    }

    QMouseEvent move(
        QEvent::MouseMove,
        QPointF(local),
        QPointF(global),
        Qt::NoButton,
        Qt::NoButton,
        QGuiApplication::keyboardModifiers());
    QCoreApplication::sendEvent(handle, &move);
}

// The view hovers no row. QAbstractItemView answers a HoverLeave by clearing
// its hovered row and repainting the rows that changed, the same path a real
// exit takes. The positions are unused there; -1, -1 says nowhere
inline void clearView(QAbstractItemView* view)
{
    QHoverEvent leave(
        QEvent::HoverLeave,
        QPointF(-1, -1),
        QPointF(-1, -1),
        QPointF(-1, -1));

    QCoreApplication::sendEvent(view->viewport(), &leave);
}

// The view hovers the row under the pointer, or none when the pointer isn't
// over its viewport. A HoverMove is what a real move sends, and the view
// answers it by hovering the row at its position (indexAt), or none
inline void syncView(QAbstractItemView* view)
{
    auto* viewport = view->viewport();
    auto global = QCursor::pos();

    if (!viewport->isVisible() || QApplication::widgetAt(global) != viewport) {
        clearView(view);
        return;
    }

    auto local = QPointF(viewport->mapFromGlobal(global));
    QHoverEvent move(QEvent::HoverMove, local, QPointF(global), local);
    QCoreApplication::sendEvent(viewport, &move);
}

namespace Internal {

// Syncs its widget's window whenever the widget moves or resizes within its
// parent, or is enabled or disabled. On the next turn of the event loop: a
// layout moves and resizes its widgets one at a time, so what's under the
// pointer is known only once all of them have
class Follower_ : public QObject
{
public:
    explicit Follower_(QWidget* parentWidget)
        : QObject(parentWidget)
        , widget_(parentWidget)
    {
        widget_->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::EnabledChange:
            Coco::Time::onNextTick(widget_, [widget = widget_] {
                sync(widget);
            });
            break;
        default:
            break;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QWidget* widget_;
};

// Syncs its view's rows on every mouse move one of the view's scroll bars is
// sent while a button is held: a drag, during which the scroll bar has the
// mouse. Seen before the scroll bar acts on the move, so a move that also
// scrolls has the view hover the row brought under the pointer afterward
// (QAbstractItemView does on every scroll). On release, Qt's own Enter or
// Leave follows the last move
class ScrollBarFollower_ : public QObject
{
public:
    explicit ScrollBarFollower_(QAbstractItemView* parentView)
        : QObject(parentView)
        , view_(parentView)
    {
        view_->verticalScrollBar()->installEventFilter(this);
        view_->horizontalScrollBar()->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::MouseMove &&
            static_cast<QMouseEvent*>(event)->buttons() != Qt::NoButton) {
            syncView(view_);
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QAbstractItemView* view_;
};

} // namespace Internal

// Syncs widget's window from now on whenever widget moves, resizes, or is
// enabled or disabled, as when a neighbor in a layout changes width under a
// still pointer. Moving because its parent moved isn't seen: call sync for
// that
inline void follow(QWidget* widget) { new Internal::Follower_(widget); }

// Keeps view's hovered row under the pointer while either of its scroll bars
// is dragged, from now on. Watches the scroll bars the view has now, so a view
// given new ones later (setVerticalScrollBar) calls this again
inline void followScrollBars(QAbstractItemView* view)
{
    new Internal::ScrollBarFollower_(view);
}

} // namespace Fernanda::Ui::HoverSync
