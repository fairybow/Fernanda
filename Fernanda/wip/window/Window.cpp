#include <functional>

#include <QCloseEvent>
#include <QMainWindow>
#include <QPointer>

#include "Coco/Debug.h"

#include "Window.h"
#include "WindowManager.h"

Window::Window(QWidget* parent)
    : QMainWindow(parent)
{
}

Window::~Window()
{
    COCO_TRACER;
    emit destroyed(this);
}

void Window::closeEvent(QCloseEvent* event)
{
    auto accepted = true;

    if (closeAcceptor_)
    {
        accepted = closeAcceptor_(this);
    }
    else if (manager_ && manager_->closeAcceptor())
    {
        accepted = manager_->closeAcceptor()(this);
    }

    accepted
        ? QMainWindow::closeEvent(event)
        : event->ignore();
}
