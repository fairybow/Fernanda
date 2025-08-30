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
