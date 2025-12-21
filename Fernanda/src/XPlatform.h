/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#if defined(Q_OS_WIN)
#    include <Windows.h>
#elif defined(Q_OS_LINUX)
//...
#elif defined(Q_OS_MACOS)
//...
#endif

#include <QList>

#include "Window.h"

// Cross-platform functions
namespace Fernanda::XPlatform {

// Lifts Windows to just below the top Window (like QWidget::stackUnder, which
// only works with sibling widgets)
inline void stackUnder(const QList<Window*>& windows, Window* top)
{
    if (!top || windows.isEmpty()) return;

#if defined(Q_OS_WIN)

    static auto get_handle = [](const Window* window) -> HWND {
        return reinterpret_cast<HWND>(window->winId());
    };

    // Get the HWND of the top window
    auto top_handle = get_handle(top);

    // Gather HWNDs of all windows in the group
    QList<HWND> handles{};

    for (auto& window : windows) {
        if (!window || window == top) continue;
        handles << get_handle(window);
    }

    // Stack all windows in order below the top window
    for (auto& handle : handles) {
        SetWindowPos(
            handle,
            top_handle,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

#elif defined(Q_OS_LINUX)

    //...

#elif defined(Q_OS_MACOS)

    //...

#endif
}

} // namespace Fernanda::XPlatform
