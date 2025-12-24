/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include <QObject>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// Application window designed to function as a sibling to other Windows via
// their close relationships with WindowService
class Window : public QMainWindow
{
    Q_OBJECT

public:
    friend class WindowService;

    explicit Window(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
    }

    virtual ~Window() override
    {
        TRACER;

        // So we can emit via Bus::windowDestroyed without decay
        emit destroyed(this);
    }

    void activate()
    {
        if (isMinimized()) showNormal();
        raise();
        activateWindow();
    }

    /// TODO TOGGLES
    bool isClosing() const noexcept { return isClosing_; }

signals:
    void destroyed(Window*);

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    // Only assigned by WindowService
    WindowService* service_ = nullptr;
    bool isClosing_ = false; /// TODO TOGGLES
};

} // namespace Fernanda
