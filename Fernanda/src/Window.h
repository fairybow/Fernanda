#pragma once

#include <functional>

#include <QCloseEvent>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

namespace Fernanda {

// Application window designed to function as a sibling to other Windows via
// their close relationships with WindowService
class Window : public QMainWindow
{
    Q_OBJECT

public:
    friend class WindowService;
    using CloseAcceptor = std::function<bool(Window*)>;

    explicit Window(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
    }

    virtual ~Window() override
    {
        COCO_TRACER;
        emit destroyed(this);
    }

    void activate()
    {
        if (isMinimized()) showNormal();
        raise();
        activateWindow();
    }

    CloseAcceptor closeAcceptor() const noexcept { return closeAcceptor_; }

    void setCloseAcceptor(const CloseAcceptor& closeAcceptor)
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

signals:
    void destroyed(Window*);

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    // Only assigned by WindowService
    QPointer<WindowService> windowService_ = nullptr;
    CloseAcceptor closeAcceptor_ = nullptr;
};

} // namespace Fernanda
