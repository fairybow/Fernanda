/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

namespace Suzuri {

inline constexpr auto WORKSPACE_FILE_NAME = "workspace.json";

// Whether a stable build is offered prereleases when it checks for updates. A
// prerelease build always is (see UpdateCheck::availableUpdate)
inline constexpr bool STABLE_BUILDS_SEE_PRERELEASES = true;

} // namespace Suzuri
