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

void Window::closeEvent(QCloseEvent* event) { QMainWindow::closeEvent(event); }

} // namespace Fernanda

/// TODO CR: Old code:

/*void Window::closeEvent(QCloseEvent* event)
{
    auto accepted = true;

    if (windowService_)
        if (auto acceptor = windowService_->closeAcceptor())
            accepted = acceptor(this);

    accepted ? QMainWindow::closeEvent(event) : event->ignore();
}*/
