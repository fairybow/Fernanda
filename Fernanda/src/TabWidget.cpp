/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "TabWidget.h"

#include <QCursor>
#include <QEnterEvent>
#include <QEvent>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QWidget>

#include "Application.h"
#include "TabWidgetCloseButton.h"
#include "TabWidgetTabBar.h"

namespace Fernanda {

bool TabWidget::eventFilter(QObject* watched, QEvent* event)
{
    // Handle mouse events on the tab bar for dragging
    if (tabsDraggable_ && watched == tabBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->button() == Qt::LeftButton)
                dragStartPosition_ = mouse_event->pos();
        } else if (event->type() == QEvent::MouseMove) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->buttons() & Qt::LeftButton) {
                auto delta = mouse_event->pos() - dragStartPosition_;

                // Only start drag if we've moved far enough VERTICALLY.
                // This allows horizontal movement to use the tab bar's
                // natural reordering

                // This works well, may want to increase required distance.
                if (qAbs(delta.y()) >= Application::startDragDistance() * 1.5) {
                    auto index = tabBar_->tabAt(dragStartPosition_);

                    if (index > -1) {
                        startDrag_(index);
                        return true; // Prevent further processing.
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool TabWidget::isDesktopDrop_() const
{
    auto final_pos = QCursor::pos();

    // Check if mouse is over any application window
    for (auto& widget : Application::topLevelWidgets())
        if (auto window = qobject_cast<QWidget*>(widget))
            if (window->isVisible() && window->geometry().contains(final_pos))
                return false; // Mouse is over an application window

    return true; // Mouse is outside all application windows
}

// TODO: Review
void TabWidget::updateMouseHoverAfterLayoutChange_()
{
    // Get current global mouse position
    auto global_mouse_pos = QCursor::pos();

    // Find which widget is actually under the mouse now
    auto widget_under_mouse = Application::widgetAt(global_mouse_pos);
    auto close_button_under_mouse =
        qobject_cast<TabWidgetCloseButton*>(widget_under_mouse);

    // Update all close buttons' hover states
    for (auto& button : closeButtons_) {
        if (!button) continue;

        if (button == close_button_under_mouse) {
            // This button should be in hover state but might not be
            if (!button->underMouse()) {
                auto local_pos = button->mapFromGlobal(global_mouse_pos);
                QEnterEvent enter_event(local_pos, local_pos, global_mouse_pos);
                Application::sendEvent(button, &enter_event);
            }
        } else {
            // This button should not be in hover state
            if (button->underMouse()) {
                QEvent leave_event(QEvent::Leave);
                Application::sendEvent(button, &leave_event);
            }
        }
    }
}

} // namespace Fernanda
