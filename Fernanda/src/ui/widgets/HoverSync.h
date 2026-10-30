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

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QWidget>
#include <QWindow>

#include <Coco/Time.h>

// Keeps hover washes in step with the pointer when widgets move under a
// pointer that stays put.
//
// Qt works out which widget is under the mouse (Qt::WA_UnderMouse, which a
// hover wash is drawn from) only when the mouse moves, or a widget is shown or
// hidden. A widget that moves or resizes while the pointer stays put keeps the
// mark it had: a toggle moving it away, or a neighbor narrowing and handing it
// room. It keeps its wash, or lacks one, until the next move. A widget
// disabled at the time gets no mark at all as the pointer passes.
//
// sync brings that next move forward: it hands the window a mouse move at the
// pointer, so Qt sends the Enter and Leave events a real move would. Setting
// the flag by hand would leave Qt's own record of the widget under the mouse
// behind, and the widget that Qt never saw entered would never be sent the
// Leave that clears it
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

} // namespace Internal

// Syncs widget's window from now on whenever widget moves, resizes, or is
// enabled or disabled, as when a neighbor in a layout changes width under a
// still pointer. Moving because its parent moved isn't seen: call sync for
// that
inline void follow(QWidget* widget) { new Internal::Follower_(widget); }

} // namespace Fernanda::Ui::HoverSync
