/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QEvent>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QString>
#include <QWidget>
#include <QtTypes>

#include "Coco/Bool.h"
#include "Coco/Utility.h"

#include "AbstractService.h"
#include "Bus.h"
#include "Commands.h"
#include "Debug.h"
#include "Version.h"
#include "Window.h"
#include "XPlatform.h"

namespace Fernanda {

// Manages Window lifecycle, creation/destruction, focus tracking, z-order
// management, and window cycling with configurable close handling across the
// Workspace. It allows our application windows to function as siblings (but
// without parentage, since QMainWindow cannot be parented by QObject, i.e., our
// Workspaces)
class WindowService : public AbstractService
{
    Q_OBJECT

public:
    friend class Window;

    using CanCloseHook = std::function<bool(Window*)>;
    using CanCloseAllHook = std::function<bool(const QList<Window*>&)>;

    WindowService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~WindowService() override { TRACER; }

    DECLARE_HOOK_ACCESSORS(
        CanCloseHook,
        canCloseHook,
        setCanCloseHook,
        canCloseHook_);

    DECLARE_HOOK_ACCESSORS(
        CanCloseAllHook,
        canCloseAllHook,
        setCanCloseAllHook,
        canCloseAllHook_);

    bool closeAll()
    {
        auto windows = this->windows();

        if (canCloseAllHook_ && !canCloseAllHook_(windows)) return false;

        isBatchClose_ = true;
        for (auto& window : windows)
            window->close();
        isBatchClose_ = false;

        return true;
    }

    int count() const { return static_cast<int>(unorderedWindows_.count()); }
    Window* active() const { return activeWindow_.get(); }

    // Highest window is first
    QList<Window*> windows() const
    {
        // Whenever a window becomes active (is on top), it's always removed (if
        // applicable) and re-added to the zOrderedVolatileWindows_ list. So,
        // our windows are ordered there from bottom (index 0) to top (index n).

        QList<Window*> list{};
        auto it = zOrderedVolatileWindows_.crbegin();
        auto end = zOrderedVolatileWindows_.crend();

        for (; it != end; ++it)
            if (*it) list << *it;

        return list;
    }

    // QList<Window*> rWindows() const { return zOrderedVolatileWindows_; }

    Window* newWindow()
    {
        auto window = make_();
        if (window) {
            window->setWindowTitle(windowTitle_());
            window->setGeometry(nextWindowGeometry_());
            window->show();
        }

        return window;
    }

    void setFlagged(bool flagged)
    {
        windowsFlagged_ = flagged;
        setAllTitles_();
    }

    void setSubtitle(const QString& subtitle)
    {
        windowsSubtitle_ = subtitle;
        setAllTitles_();
    }

signals:
    void lastWindowClosed();

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Commands::NEW_WINDOW, [&] {
            return newWindow();
        });

        bus->addCommandHandler(Commands::WINDOWS_SET, [&] {
            return unorderedWindows_;
        });

        bus->addCommandHandler(Commands::WINDOWS, [&] { return windows(); });

        bus->addCommandHandler(Commands::CLOSE_ALL_WINDOWS, [&] {
            return closeAll();
        });
    }

    virtual void connectBusEvents() override
    {
        //...
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::WindowActivate) {

            if (auto active_window = qobject_cast<Window*>(watched)) {
                setActiveWindow_(active_window);
                XPlatform::stackUnder(zOrderedVolatileWindows_, active_window);
            }

        } else if (
            event->type() == QEvent::Show || event->type() == QEvent::Hide) {

            // if (auto window = qobject_cast<Window*>(watched))
            // emit bus->visibleWindowCountChanged(visibleCount_());

        } else if (event->type() == QEvent::Close) {

            //...
        }

        return QObject::eventFilter(watched, event);
    }

private:
    static constexpr auto DEFAULT_GEOMETRY_ = QRect{ 100, 100, 600, 500 };
    static constexpr auto GEOMETRY_OFFSET_ = 50;

    QList<Window*> zOrderedVolatileWindows_{}; // Highest window is always last
    QSet<Window*> unorderedWindows_{};
    QPointer<Window> activeWindow_ = nullptr;
    QPointer<Window> lastFocusedAppWindow_ = nullptr;

    bool isBatchClose_ = false;
    CanCloseHook canCloseHook_ = nullptr;
    CanCloseAllHook canCloseAllHook_ = nullptr;

    bool windowsFlagged_ = false;
    QString windowsSubtitle_{};

    void setup_();

    Window* make_()
    {
        auto window = new Window(nullptr);
        window->setAttribute(Qt::WA_DeleteOnClose);

        zOrderedVolatileWindows_ << window;
        unorderedWindows_ << window;

        window->service_ = this;
        window->installEventFilter(this);

        connect(
            window,
            &Window::destroyed,
            this,
            &WindowService::onWindowDestroyed_);

        INFO("Window created [{}]", window);
        emit bus->windowCreated(window);

        return window;
    }

    QString windowTitle_() const
    {
        // * subtitle - title
        QString title = windowsFlagged_ ? "* " : "";
        if (!windowsSubtitle_.isEmpty()) title += windowsSubtitle_ + " - ";
        title += VERSION_APP_NAME_STRING;
        return title;
    }

    void setAllTitles_()
    {
        auto title = windowTitle_();
        for (auto& window : unorderedWindows_)
            if (window) window->setWindowTitle(title);
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

    // These are windows that have called Window::show() (Don't mistake this as
    // dealing with minimization!
    // TODO: Ensure this is needed
    QList<Window*> visibleWindows_() const
    {
        QList<Window*> visible{};

        for (auto& window : zOrderedVolatileWindows_)
            if (window && window->isVisible()) visible << window;

        return visible;
    }

    // Can be set to nullptr
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
    }

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
                // I am not entirely sure my logic/reasoning for tracking an
                // app-wide active window is correct or necessary, but so far
                // everything seems to work okay...
                lastFocusedAppWindow_->activate();

            } else {

                // If this Manager is not empty, then it should resume focus. If
                // it is, Qt should have taken care of refocusing
                auto& next_window = zOrderedVolatileWindows_.last();
                next_window->activate();
            }
        }

        INFO("Window destroyed [{}]", window);
        emit bus->windowDestroyed(window);

        if (last_window_closed) {
            INFO("Last window closed");
            emit lastWindowClosed();
        }
    }

    void onApplicationFocusChanged_(QWidget* old, QWidget* now)
    {
        (void)now;
        if (!old) return;
        if (auto window = qobject_cast<Window*>(old))
            lastFocusedAppWindow_ = window;
    }
};

} // namespace Fernanda

/// Old:

/*
// TODO: Ensure this is needed
int visibleCount_() const
{
    auto i = 0;

    for (auto& window : unorderedWindows_)
        if (window && window->isVisible()) ++i;

    return i;
}

// in set active window (end):
emit bus->activeWindowChanged(activeWindow_.get());

void bubbleDelay_(unsigned int msecs) const;

// https://stackoverflow.com/a/11487434
// Questionable
void WindowService::bubbleDelay_(unsigned int msecs) const
{
    auto die_time = QTime::currentTime().addMSecs(msecs);

    while (QTime::currentTime() < die_time)
        Application::processEvents(QEventLoop::AllEvents, 100);
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

QSet<Window*> windowsUnordered_() const noexcept
{
    return unorderedWindows_;
}

// Cycling:

    // Window cycling
    QList<Window*> cyclingOrder_{};
    qsizetype currentCyclingIndex_ = -1;
    Debouncer* cycleDebouncer_ = new Debouncer(2000, this, [&] {
        currentCyclingIndex_ = -1;
        cyclingOrder_.clear();
    });

    // void startOrContinueCycling_()
    //{
    //     if (cyclingOrder_.isEmpty()) {
    //         cyclingOrder_ = visibleWindows_(); // Snapshot the current order
    //         currentCyclingIndex_ =
    //             activeWindow_ ? cyclingOrder_.indexOf(activeWindow_) : 0;
    //         if (currentCyclingIndex_ < 0) currentCyclingIndex_ = 0;
    //     }

    //    cycleDebouncer_->start();
    //}

    // void activatePrevious_()
    //{
    //     if (windows_.count() <= 1) return;

    //    startOrContinueCycling_();

    //    // Move to previous window using our independent index
    //    currentCyclingIndex_ = (currentCyclingIndex_ - 1 +
    //    cyclingOrder_.size())
    //                           % cyclingOrder_.size();

    //    // Activate the window at the new position
    //    if (currentCyclingIndex_ < cyclingOrder_.size()) {
    //        cyclingOrder_[currentCyclingIndex_]->activate();
    //    }
    //}

    // void activateNext_()
    //{
    //     if (windows_.count() <= 1) return;

    //    startOrContinueCycling_();

    //    // Move to next window using our independent index
    //    currentCyclingIndex_ =
    //        (currentCyclingIndex_ + 1) % cyclingOrder_.size();

    //    // Activate the window at the new position
    //    if (currentCyclingIndex_ < cyclingOrder_.size()) {
    //        cyclingOrder_[currentCyclingIndex_]->activate();
    //    }
    //}

*/
