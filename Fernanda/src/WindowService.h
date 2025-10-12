/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QEvent>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QWidget>
#include <QtTypes>

#include "Coco/Bool.h"
#include "Coco/Utility.h"

#include "Bus.h"
#include "Debouncer.h"
#include "Debug.h"
#include "IService.h"
#include "Utility.h"
#include "Window.h"
#include "XPlatform.h"

namespace Fernanda {

// Manages Window lifecycle, creation/destruction, focus tracking, z-order
// management, and window cycling with configurable close handling across the
// Workspace. It allows our application windows to function as siblings (but
// without parentage, since QMainWindow cannot be parented by QObject, i.e., our
// Workspaces)
class WindowService : public IService
{
    Q_OBJECT

public:
    WindowService(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        setup_();
    }

    virtual ~WindowService() override { TRACER; }

    Window::CloseAcceptor closeAcceptor() const noexcept
    {
        return closeAcceptor_;
    }

    void setCloseAcceptor(const Window::CloseAcceptor& closeAcceptor)
    {
        closeAcceptor_ = closeAcceptor;
    }

    template <typename ClassT>
    void setCloseAcceptor(ClassT* object, bool (ClassT::*method)(Window*))
    {
        closeAcceptor_ = [object, method](Window* window) {
            return (object->*method)(window);
        };
    }

    void showAll() const
    {
        // Because the Window list is volatile, we use a copy
        for (auto& window : windows_())
            window->show();
    }

    void bubbleShow(unsigned int delayMsecs = 50) const
    {
        for (auto& window : windows_()) {
            window->show();
            bubbleDelay_(delayMsecs);
        }
    }

    void activateAll() const
    {
        if (!activeWindow_) return;
        activeWindow_->activate(); // Stack under will take effect
    }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        //...
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::WindowActivate) {

            if (auto active_window = cast<Window*>(watched)) {
                setActiveWindow_(active_window);
                XPlatform::stackUnder(zOrderedVolatileWindows_, active_window);
            }

        } else if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {

            if (auto window = cast<Window*>(watched))
                emit bus->visibleWindowCountChanged(visibleCount_());

        } else if (event->type() == QEvent::Close) {

            //...

        }

        return QObject::eventFilter(watched, event);
    }

private:
    static constexpr auto DEFAULT_GEOMETRY_ = QRect{ 100, 100, 600, 500 };
    static constexpr auto GEOMETRY_OFFSET_ = 50;

    Window::CloseAcceptor closeAcceptor_ = nullptr;

    QList<Window*> zOrderedVolatileWindows_{}; // Highest window is always last
    QSet<Window*> unorderedWindows_{};
    QPointer<Window> activeWindow_ = nullptr;
    QPointer<Window> lastFocusedAppWindow_ = nullptr;

    // Window cycling
    /*
    QList<Window*> cyclingOrder_{};
    qsizetype currentCyclingIndex_ = -1;
    Debouncer* cycleDebouncer_ = new Debouncer(2000, this, [&] {
        currentCyclingIndex_ = -1;
        cyclingOrder_.clear();
    });
    */

    void setup_();

    // TODO: Just a command that returns the member...
    // Window* active() const noexcept { return activeWindow_; }

    Window* make_(const QRect& geometry = {})
    {
        auto window = new Window(nullptr);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setGeometry(
            geometry.isNull() ? nextWindowGeometry_() : geometry);

        zOrderedVolatileWindows_ << window;
        unorderedWindows_ << window;

        window->windowService_ = this;
        window->installEventFilter(this);

        connect(
            window,
            &Window::destroyed,
            this,
            &WindowService::onWindowDestroyed_);

        emit bus->windowCreated(window);
        return window;
    }

    Window* make_(const QPoint& pos)
    {
        auto geometry = nextWindowGeometry_();
        geometry.moveTo(pos);
        return make_(geometry);
    }

    QList<Window*> windows_() const
    {
        QList<Window*> list{};

        for (const auto& window : zOrderedVolatileWindows_)
            if (window) list << window;

        return list;
    }

    // Highest is first
    QList<Window*> windowsReversed_() const
    {
        QList<Window*> list{};
        auto it = zOrderedVolatileWindows_.crbegin();
        auto end = zOrderedVolatileWindows_.crend();

        for (; it != end; ++it)
            if (*it) list << *it;

        return list;
    }

    //QSet<Window*> windowsUnordered_() const noexcept
    //{
    //    return unorderedWindows_;
    //}

    int visibleCount_() const
    {
        auto i = 0;

        for (auto& window : unorderedWindows_)
            if (window && window->isVisible()) ++i;

        return i;
    }

    void bubbleDelay_(unsigned int msecs) const;

    // Note: Can be set to nullptr
    void setActiveWindow_(Window* activeWindow)
    {
        if (activeWindow_ == activeWindow) return;
        activeWindow_ = activeWindow;

        // No need to re-order if there's no, or only one, window
        if (activeWindow && zOrderedVolatileWindows_.size() > 1) {
            // Keep an internal z-order
            zOrderedVolatileWindows_.removeAll(activeWindow);
            zOrderedVolatileWindows_ << activeWindow;
        }

        emit bus->activeWindowChanged(activeWindow_);
    }

    // These are windows that have called Window::show() (Don't mistake this as
    // dealing with minimization!
    QList<Window*> visibleWindows_() const
    {
        QList<Window*> visible{};

        for (auto& window : zOrderedVolatileWindows_)
            if (window && window->isVisible()) visible << window;

        return visible;
    }

    QRect nextWindowGeometry_() const
    {
        if (activeWindow_) {
            auto& geometry = activeWindow_->geometry();

            return { geometry.x() + GEOMETRY_OFFSET_,
                     geometry.y() + GEOMETRY_OFFSET_,
                     geometry.width(),
                     geometry.height() };
        }

        return DEFAULT_GEOMETRY_;
    }

    //void startOrContinueCycling_()
    //{
    //    if (cyclingOrder_.isEmpty()) {
    //        cyclingOrder_ = visibleWindows_(); // Snapshot the current order
    //        currentCyclingIndex_ =
    //            activeWindow_ ? cyclingOrder_.indexOf(activeWindow_) : 0;
    //        if (currentCyclingIndex_ < 0) currentCyclingIndex_ = 0;
    //    }

    //    cycleDebouncer_->start();
    //}

    //void activatePrevious_()
    //{
    //    if (windows_.count() <= 1) return;

    //    startOrContinueCycling_();

    //    // Move to previous window using our independent index
    //    currentCyclingIndex_ = (currentCyclingIndex_ - 1 + cyclingOrder_.size())
    //                           % cyclingOrder_.size();

    //    // Activate the window at the new position
    //    if (currentCyclingIndex_ < cyclingOrder_.size()) {
    //        cyclingOrder_[currentCyclingIndex_]->activate();
    //    }
    //}

    //void activateNext_()
    //{
    //    if (windows_.count() <= 1) return;

    //    startOrContinueCycling_();

    //    // Move to next window using our independent index
    //    currentCyclingIndex_ =
    //        (currentCyclingIndex_ + 1) % cyclingOrder_.size();

    //    // Activate the window at the new position
    //    if (currentCyclingIndex_ < cyclingOrder_.size()) {
    //        cyclingOrder_[currentCyclingIndex_]->activate();
    //    }
    //}

private slots:
    void onWindowDestroyed_(Window* window)
    {
        if (!window) return;

        zOrderedVolatileWindows_.removeAll(window);
        unorderedWindows_.remove(window);

        auto last_window_closed = false;

        if (zOrderedVolatileWindows_.isEmpty()) {
            setActiveWindow_(nullptr);
            last_window_closed = true;

            // Let Qt focus the next window from another WindowManager, if any

        } else {

            // Qt will return focus to the previously activated window. If that
            // previously active window was in another WindowManager, then we
            // need to prevent that to prevent flickering. Plus, it makes sense
            // to simply activate the next highest window in the current
            // WindowManager. However, if the previously active window was in
            // the same WindowManager, then we should activate that one instead
            if (lastFocusedAppWindow_
                && zOrderedVolatileWindows_.contains(lastFocusedAppWindow_)) {
                // Note: I am not entirely sure my logic/reasoning for tracking
                // an app-wide active window is correct or necessary, but so far
                // everything seems to work okay...
                lastFocusedAppWindow_->activate();
            } else {
                // If this Manager is not empty, then it should resume focus. If
                // it is, Qt should have taken care of refocusing
                auto& next_window = zOrderedVolatileWindows_.last();
                next_window->activate();
            }

        }

        emit bus->windowDestroyed(window);
        if (last_window_closed) emit bus->lastWindowClosed();
    }

    void onApplicationFocusChanged_(QWidget* old, QWidget* now)
    {
        (void)now;
        if (!old) return;
        if (auto window = cast<Window*>(old)) lastFocusedAppWindow_ = window;
    }
};

} // namespace Fernanda
