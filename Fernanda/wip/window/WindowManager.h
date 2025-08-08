#pragma once

#include <type_traits>

#include <QEvent>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QtTypes>
#include <QWidget>

#include "Coco/Bool.h"
#include "Coco/Utility.h"

#include "Window.h"

class WindowManager : public QObject
{
    Q_OBJECT

public:
    COCO_BOOL(HaltOnRefusal);

    explicit WindowManager(QObject* parent = nullptr);
    virtual ~WindowManager() override;

    bool stackUnder() const noexcept { return stackUnder_; }
    void setStackUnder(bool stackUnder) { stackUnder_ = stackUnder; }

    Window::CloseAcceptor closeAcceptor() const noexcept { return closeAcceptor_; }
    void setCloseAcceptor(const Window::CloseAcceptor& closeAcceptor) { closeAcceptor_ = closeAcceptor; }

    template <typename ClassT>
    void setCloseAcceptor(ClassT* object, bool (ClassT::* method)(Window*))
    {
        closeAcceptor_ = [object, method](Window* window)
            {
                return (object->*method)(window);
            };
    }

    Window* active() const noexcept { return activeWindow_; }

    template<WindowPointer T>
    T active() const { return qobject_cast<T>(activeWindow_); }

    template<WindowPointer T = Window*>
    T make(const QRect& geometry = {})
    {
        // Want to make sure if using Window subclass that we initialize it as
        // such, so no extra non-template returner here (unlike above)
        auto window = new std::remove_pointer_t<T>{};
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setGeometry(geometry.isNull() ? nextWindowGeometry_() : geometry);
        add(window);
        return window;
    }

    template<WindowPointer T = Window*>
    T make(const QPoint& pos)
    {
        auto geometry = nextWindowGeometry_();
        geometry.moveTo(pos);
        return make<T>(geometry);
    }

    void showAll() const;
    void bubbleShow(unsigned int delayMsecs = 50) const;
    void closeAll(HaltOnRefusal haltOnRefusal = HaltOnRefusal::No);

    void deleteAll();
    void deleteAllLater();

    void add(Window* window);

    template<WindowPointer T = Window*>
    QList<T> windows() const
    {
        QList<T> list{};

        for (const auto& window : zOrderedVolatileWindows_)
            if (auto casted = qobject_cast<T>(window))
                list << casted;

        return list;
    }

    template<WindowPointer T = Window*>
    QList<T> windowsReversed() const
    {
        QList<T> list{};
        auto it = zOrderedVolatileWindows_.crbegin();
        auto end = zOrderedVolatileWindows_.crend();

        for (; it != end; ++it)
            if (auto casted = qobject_cast<T>(*it))
                list << casted;

        return list;
    }

    void activatePrevious()
    {
        if (!activeWindow_) return;

        auto windows = visibleWindows_();
        if (windows.count() <= 1) return;

        auto i = windows.indexOf(activeWindow_);

        if (i < 0)
        {
            windows.first()->activate();
            return;
        }

        // Move to previous window, wrapping around to end if at beginning
        int previous = (i - 1 + windows.size()) % windows.size();
        windows[previous]->activate();
    }

    void activateNext()
    {
        if (!activeWindow_) return;

        auto windows = visibleWindows_();
        if (windows.count() <= 1) return;

        auto i = windows.indexOf(activeWindow_);

        if (i < 0)
        {
            windows.first()->activate();
            return;
        }

        // Move to next window, wrapping around to beginning if at end
        int next = (i + 1) % windows.size();
        windows[next]->activate();
    }

    int count() const noexcept { return windows_.count(); }
    qsizetype size() const noexcept { return windows_.size(); }
    bool contains(Window* const& window) const noexcept { return windows_.contains(window); }

signals:
    void activeWindowChanged(Window* window);
    void lastWindowClosed();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

private:
    static constexpr auto DEFAULT_GEOMETRY_ = QRect{ 100, 100, 600, 500 };
    static constexpr auto GEOMETRY_OFFSET_ = 50;
    bool stackUnder_ = true;
    Window::CloseAcceptor closeAcceptor_ = nullptr;
    QList<Window*> zOrderedVolatileWindows_{}; // Highest window is always last
    QList<Window*> windows_{};
    QPointer<Window> activeWindow_ = nullptr;
    QPointer<Window> lastFocusedAppWindow_ = nullptr;

    void setActiveWindow_(Window* activeWindow);

    QList<Window*> visibleWindows_() const
    {
        QList<Window*> visible{};

        for (auto& window : windows_)
            if (window && window->isVisible())
                visible << window;

        return visible;
    }

    QRect nextWindowGeometry_() const
    {
        if (activeWindow_)
        {
            auto& geometry = activeWindow_->geometry();

            return
            {
                geometry.x() + GEOMETRY_OFFSET_,
                geometry.y() + GEOMETRY_OFFSET_,
                geometry.width(),
                geometry.height()
            };
        }

        return DEFAULT_GEOMETRY_;
    }

private slots:
    void onWindowDestroyed_(Window* window);
    void onQApplicationFocusChanged_(QWidget* old, QWidget* now);
};
