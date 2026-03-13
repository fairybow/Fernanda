/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "ui/Window.h"

#include <QCloseEvent>
#include <QMainWindow>

#include "services/WindowService.h"

namespace Fernanda {

void Window::closeEvent(QCloseEvent* event)
{
    if (!service_ || service_->isBatchClose_ || service_->isDeferredClose_) {
        QMainWindow::closeEvent(event);
        return;
    }

    // Defer to coalesce with any other close events in this tick
    event->ignore();
    service_->deferClose_(this);
}

} // namespace Fernanda
