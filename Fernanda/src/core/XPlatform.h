/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QList>

#if defined(Q_OS_WIN)
#    include <Windows.h>
#elif defined(Q_OS_LINUX)
#    include <QGuiApplication>
#    include <xcb/xcb.h>
#endif

#include "ui/Window.h"

// Cross-platform functions
namespace Fernanda::XPlatform {

namespace Internal {

#if defined(Q_OS_MACOS)
    void stackUnder_macOS_(const QList<Window*>& windows, Window* top);
#endif

} // namespace Internal

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

    /// TODO XP: Untested!

    // Z-ordering isn't exposed to us on Wayland (not XWayland), is my
    // understanding, so this function (or something similar) isn't possible

    auto* x11_app =
        qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11_app) return; // Wayland or no X11 connection

    auto* connection = x11_app->connection();
    if (!connection) return;

    auto top_id = static_cast<xcb_window_t>(top->winId());

    for (auto& window : windows) {
        if (!window || window == top) continue;

        auto win_id = static_cast<xcb_window_t>(window->winId());
        uint32_t values[] = { top_id, XCB_STACK_MODE_BELOW };

        xcb_configure_window(
            connection,
            win_id,
            XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
            values);
    }

    xcb_flush(connection);

#elif defined(Q_OS_MACOS)

    /// TODO XP: Untested!
    Internal::stackUnder_macOS_(windows, top);

#endif
}

} // namespace Fernanda::XPlatform
