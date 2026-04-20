/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "services/WindowService.h"

#include "core/Application.h"
#include "workspaces/Bus.h"

namespace Fernanda {

void WindowService::setup_()
{
    connect(
        app(),
        &Application::focusChanged,
        this,
        &WindowService::onApplicationFocusChanged_);
}

} // namespace Fernanda
