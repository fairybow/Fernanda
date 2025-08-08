#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QEvent>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QTime>
#include <QWidget>

#include "Coco/Debug.h"

#include "WindowManager.h"
#include "XPlatform.h"

// Note: https://stackoverflow.com/a/11487434
static void delay_(unsigned int msecs)
{
    auto die_time = QTime::currentTime().addMSecs(msecs);

    while (QTime::currentTime() < die_time)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

WindowManager::WindowManager(QObject* parent)
    : QObject(parent)
{
    connect
    (
        qApp,
        &QApplication::focusChanged,
        this,
        &WindowManager::onQApplicationFocusChanged_
    );
}

WindowManager::~WindowManager() { COCO_TRACER; }

void WindowManager::showAll() const
{
    // Because the Window list is volatile, we use a copy
    for (auto& window : windows())
        window->show();
}

void WindowManager::bubbleShow(unsigned int delayMsecs) const
{
    for (auto& window : windows())
    {
        window->show();
        delay_(delayMsecs);
    }
}

void WindowManager::closeAll(HaltOnRefusal haltOnRefusal)
{
    for (auto& window : windowsReversed())
        if (!window->close() && haltOnRefusal)
            return;
}

void WindowManager::deleteAll()
{
    for (auto& window : windowsReversed())
        delete window;
}

void WindowManager::deleteAllLater()
{
    for (auto& window : windowsReversed())
        window->deleteLater();
}

void WindowManager::add(Window* window)
{
    if (window->parent())
        window->setParent(nullptr);

    zOrderedVolatileWindows_ << window;
    windows_ << window;
    window->manager_ = this;
    window->installEventFilter(this);

    connect
    (
        window,
        &Window::destroyed,
        this,
        &WindowManager::onWindowDestroyed_
    );
}

// Note: If we are not modifying or consuming the event, we should always return
//       the default filter result
bool WindowManager::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        if (auto active_window = qobject_cast<Window*>(watched))
        {
            setActiveWindow_(active_window);

            if (stackUnder_)
                XPlatform::stackUnder(zOrderedVolatileWindows_, active_window);
        }
    }

    return QObject::eventFilter(watched, event);
}

// Note: Can be nullptr
void WindowManager::setActiveWindow_(Window* activeWindow)
{
    if (activeWindow_ == activeWindow) return;
    activeWindow_ = activeWindow;

    // No need to re-order if there's no, or only one, window
    if (activeWindow && zOrderedVolatileWindows_.size() > 1)
    {
        // Keep an internal z-order
        zOrderedVolatileWindows_.removeAll(activeWindow);
        zOrderedVolatileWindows_ << activeWindow;
    }

    emit activeWindowChanged(activeWindow_);
}

void WindowManager::onWindowDestroyed_(Window* window)
{
    zOrderedVolatileWindows_.removeAll(window);
    windows_.removeAll(window);

    if (zOrderedVolatileWindows_.isEmpty())
    {
        setActiveWindow_(nullptr);
        emit lastWindowClosed();

        // Let Qt focus the next window from another WindowManager, if any
    }
    else // !zOrderedVolatileWindows_.isEmpty()
    {
        // Qt will return focus to the previously activated window. If that
        // previously active window was in another WindowManager, then we need
        // to prevent that to prevent flickering. Plus, it makes sense to simply
        // activate the next highest window in the current WindowManager.
        // However, if the previously active window was in the same
        // WindowManager, then we should activate that one instead
        if (lastFocusedAppWindow_ && zOrderedVolatileWindows_.contains(lastFocusedAppWindow_))
        {
            // Note: I am not entirely sure my logic/reasoning for tracking an
            //       app-wide active window is correct or necessary, but so far
            //       everything seems to work okay...
            lastFocusedAppWindow_->activate();
        }
        else
        {
            // If this Manager is not empty, then it should resume focus. If it
            // is, Qt should have taken care of refocusing
            auto& next_window = zOrderedVolatileWindows_.last();
            next_window->activate();
        }
    }
}

void WindowManager::onQApplicationFocusChanged_(QWidget* old, QWidget* now)
{
    (void)now;

    if (!old) return;

    if (auto window = qobject_cast<Window*>(old))
        lastFocusedAppWindow_ = window;
}
