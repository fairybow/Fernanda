/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

// Note: Visual Studio automatically defines _DEBUG in Debug builds
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define VERSION_DEBUG
#endif

#define VERSION_PRERELEASE // Toggle manually

/// ======================================================================== ///
/// *** VERSION INFO ***                                                     ///
/// ======================================================================== ///

#define VERSION_MAJOR                           0
#define VERSION_MINOR                           99
#define VERSION_PATCH                           0
#define VERSION_PRERELEASE_STRING               "beta.1"
#define VERSION_BUILD                           1

#define VERSION_FULL                            \
        VERSION_MAJOR,                          \
        VERSION_MINOR,                          \
        VERSION_PATCH,                          \
        VERSION_BUILD

#define VERSION                                 \
        VERSION_MAJOR,                          \
        VERSION_MINOR

// Utility
#define VERSION_STRINGIFY_(x)                   #x
#define VERSION_STRINGIFY(x)                    VERSION_STRINGIFY_(x)

// Version string
#ifdef  VERSION_PRERELEASE
#define VERSION_FULL_STRING                     \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)    "." \
        VERSION_STRINGIFY(VERSION_PATCH)    "-" \
        VERSION_PRERELEASE_STRING           "+" \
        VERSION_STRINGIFY(VERSION_BUILD)
#else
#define VERSION_FULL_STRING                     \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)    "." \
        VERSION_STRINGIFY(VERSION_PATCH)    "+" \
        VERSION_STRINGIFY(VERSION_BUILD)
#endif

#define VERSION_STRING                          \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)

/// ======================================================================== ///
/// *** PUBLISHING INFO ***                                                  ///
/// ======================================================================== ///

#define VERSION_AUTHOR_STRING                   "fairybow"
#define VERSION_APP_NAME_STRING                 "Fernanda"
#define VERSION_RELEASE_NAME_STRING             "Atlas"
#define VERSION_COPYRIGHT_STRING                "Copyright \xa9 2025-2026 fairybow"
#define VERSION_DOMAIN                          "https://github.com/fairybow/Fernanda"
#define VERSION_WINDOWS_FILE_STRING             VERSION_APP_NAME_STRING ".exe"
