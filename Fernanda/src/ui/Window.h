/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include <QObject>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

class WindowService;

// Application window designed to function as a sibling to other Windows via
// their close relationships with WindowService
class Window : public QMainWindow
{
    Q_OBJECT

public:
    friend WindowService;

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

signals:
    void destroyed(Window*);

protected:
    // See: docs/Closures.md
    virtual void closeEvent(QCloseEvent* event) override;

private:
    // Only assigned by WindowService
    WindowService* service_ = nullptr;
};

} // namespace Fernanda
