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

#include "core/Version.h"

#pragma message(                                                               \
    "=========================== Build Messages ===========================")

#ifdef VERSION_DEBUG
#    pragma message("VERSION_DEBUG is defined")
#else
#    pragma message("VERSION_DEBUG is not defined")
#endif

#if VERSION_IS_PRERELEASE
#    pragma message("VERSION_IS_PRERELEASE = 1")
#else
#    pragma message("VERSION_IS_PRERELEASE = 0")
#endif

#pragma message("Version: " VERSION_FULL_STRING)
#pragma message("Release Name: \"" VERSION_RELEASE_NAME_STRING "\"")

#pragma message(                                                               \
    "======================================================================")
