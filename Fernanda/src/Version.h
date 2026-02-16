/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#define VERSION_IS_PRERELEASE 1 // 1 = prerelease, 0 = stable

namespace Fernanda::Version {

// Visual Studio automatically defines _DEBUG in Debug builds
#if defined(_DEBUG)

inline constexpr bool isDebug = true;
#    define VERSION_DEBUG

#else

inline constexpr bool isDebug = false;

#endif

#if VERSION_IS_PRERELEASE

inline constexpr bool isPrerelease = true;

#else

inline constexpr bool isPrerelease = false;

#endif

} // namespace Fernanda::Version

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
#define VERSION_PRERELEASE_STRING               "beta.1"

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
#define VERSION_APP_NAME_STRING                 "Fernanda"
#define VERSION_RELEASE_NAME_STRING             "Bashō"
#define VERSION_COPYRIGHT_STRING                "Copyright \xa9 2025-2026 fairybow"
#define VERSION_DOMAIN                          "https://github.com/fairybow/Fernanda"
#define VERSION_WINDOWS_FILE_STRING             VERSION_APP_NAME_STRING ".exe"

// Use VERSION_FULL_STRING for GitHub release tags!

// clang-format on
