/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include "Version.h"

#ifdef VERSION_DEBUG
#    pragma message("VERSION_DEBUG is defined")
#else
#    pragma message("VERSION_DEBUG is not defined")
#endif

#ifdef VERSION_PRERELEASE
#    pragma message("VERSION_PRERELEASE is defined")
#else
#    pragma message("VERSION_PRERELEASE is not defined")
#endif

#pragma message("Version: " VERSION_STRING " (" VERSION_FULL_STRING ")")
#pragma message("Release Name: \"" VERSION_RELEASE_NAME_STRING "\"")
