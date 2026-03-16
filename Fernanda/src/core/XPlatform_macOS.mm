/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "core/XPlatform.h"

#import <AppKit/AppKit.h>

namespace Fernanda::XPlatform {

/// TODO XP: Untested!
void stackUnder_macOS(const QList<Window*>& windows, Window* top)
{
    auto get_ns_window = [](const Window* window) -> NSWindow* {
        auto ns_view = reinterpret_cast<NSView*>(window->winId());
        return [ns_view window];
    };

    auto* top_ns = get_ns_window(top);
    auto top_number = [top_ns windowNumber];

    for (auto& window : windows) {
        if (!window || window == top) continue;

        auto* ns_win = get_ns_window(window);
        [ns_win orderWindow:NSWindowBelow relativeTo:top_number];
    }
}

} // namespace Fernanda::XPlatform
