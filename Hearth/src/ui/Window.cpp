/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "ui/Window.h"

#include <QCloseEvent>
#include <QMainWindow>

#include "services/WindowService.h"

namespace Hearth {

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

} // namespace Hearth
