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

#include "core/XPlatform.h"

#import <AppKit/AppKit.h>

namespace Fernanda::XPlatform::Internal {

/// TODO XP: Untested!
void stackUnder_macOS_(const QList<Window*>& windows, Window* top)
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

} // namespace Fernanda::XPlatform::Internal
