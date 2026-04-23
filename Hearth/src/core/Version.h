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

#pragma once

// Manual toggle!
#define VERSION_IS_PRERELEASE 1 // 1 = prerelease, 0 = stable

#if defined(VERSION_DEBUG)
#    define VERSION_IS_DEBUG 1
#else
#    define VERSION_IS_DEBUG 0
#endif

// clang-format off

/// ======================================================================== ///
/// *** VERSION INFO ***                                                     ///
/// ======================================================================== ///

// | Part       | When to increment                                            |
// |------------|--------------------------------------------------------------|
// | MAJOR      | Breaking changes / major release (1.0.0 = first stable)      |
// | MINOR      | New features, backward compatible                            |
// | PATCH      | Bug fixes only                                               |
// | PRERELEASE | Optional label: alpha.1, beta.1, rc.1, etc.                  |

#define VERSION_MAJOR                           0
#define VERSION_MINOR                           99
#define VERSION_PATCH                           0
#define VERSION_PRERELEASE_STRING               "beta.28"

// Major.Minor.Patch
#define VERSION_3                               \
        VERSION_MAJOR,                          \
        VERSION_MINOR,                          \
        VERSION_PATCH

// Major.Minor
#define VERSION_2                               \
        VERSION_MAJOR,                          \
        VERSION_MINOR

// Strings

#define VERSION_STRINGIFY_(x)                   #x
#define VERSION_STRINGIFY(x)                    VERSION_STRINGIFY_(x)

// Major.Minor.Patch
#define VERSION_3_STRING                        \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)    "." \
        VERSION_STRINGIFY(VERSION_PATCH)

// Major.Minor
#define VERSION_2_STRING                        \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)

#if     VERSION_IS_PRERELEASE
#define VERSION_FULL_STRING                     \
        VERSION_STRINGIFY(VERSION_MAJOR)    "." \
        VERSION_STRINGIFY(VERSION_MINOR)    "." \
        VERSION_STRINGIFY(VERSION_PATCH)    "-" \
        VERSION_PRERELEASE_STRING
#else
#    define VERSION_FULL_STRING VERSION_3_STRING
#endif

/// ======================================================================== ///
/// *** PUBLISHING INFO ***                                                  ///
/// ======================================================================== ///

#define VERSION_AUTHOR_STRING                   "fairybow"
#define VERSION_APP_NAME_STRING                 "Hearth"
#define VERSION_APP_NAME_STRING_L               "hearth"
#define VERSION_RELEASE_NAME_STRING             "Bashō"
#define VERSION_COPYRIGHT_STRING                "Copyright (C) 2025-2026 fairybow"
#define VERSION_DOMAIN                          "https://github.com/fairybow/Hearth"

#if defined(Q_OS_WIN)
#    define VERSION_FILE_STRING                 VERSION_APP_NAME_STRING ".exe"
#elif defined(Q_OS_MACOS)
#    define VERSION_FILE_STRING                 VERSION_APP_NAME_STRING ".app"
#else
#    define VERSION_FILE_STRING                 VERSION_APP_NAME_STRING
#endif

// Use VERSION_FULL_STRING for GitHub release tags!

// clang-format on
