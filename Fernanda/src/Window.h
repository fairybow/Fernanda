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

#include <QCloseEvent>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include "Coco/Layout.h"

#include "Debug.h"

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
        TRACER;
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
