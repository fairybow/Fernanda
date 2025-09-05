/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QCloseEvent>
#include <QMainWindow>

#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

void Window::closeEvent(QCloseEvent* event)
{
    auto accepted = true;

    if (closeAcceptor_) {
        accepted = closeAcceptor_(this);
    } else if (windowService_ && windowService_->closeAcceptor()) {
        accepted = windowService_->closeAcceptor()(this);
    }

    accepted ? QMainWindow::closeEvent(event) : event->ignore();
}

} // namespace Fernanda
