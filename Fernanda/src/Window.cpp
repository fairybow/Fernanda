/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Window.h"

#include <QCloseEvent>
#include <QMainWindow>

#include "WindowService.h"

namespace Fernanda {

void Window::closeEvent(QCloseEvent* event)
{
    // For batch close, WindowService uses its own hook
    if (!service_ || service_->isBatchClose_) goto close;

    // Single window close (allows us to close windows normally, via close
    // method or UI button, and still allow the Workspace to accept or reject)
    if (service_->canCloseHook_ && !service_->canCloseHook_(this)) {
        event->ignore();
        return;
    }

close:
    QMainWindow::closeEvent(event);
}

} // namespace Fernanda
