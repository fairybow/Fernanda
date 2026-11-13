/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QtGlobal>

#if defined(Q_OS_WIN)
#    include <Windows.h>
#    include <appmodel.h>
#endif

// Fernanda's MSIX package, the Microsoft Store version. The Store updates that
// version itself, so it never checks for updates
namespace Fernanda::Msix {

// Whether this process runs from an MSIX package. A packaged process has a
// package identity, which GetCurrentPackageFullName reports; for any other it
// returns APPMODEL_ERROR_NO_PACKAGE. Asked with no buffer, a packaged process
// gets ERROR_INSUFFICIENT_BUFFER instead, which is enough to tell. The identity
// is fixed when the process starts, so it's asked once. Off Windows there is no
// MSIX, so it's always false
[[nodiscard]] inline bool runsFromPackage()
{
#if defined(Q_OS_WIN)

    static const bool packaged = [] {
        UINT32 length = 0;
        return GetCurrentPackageFullName(&length, nullptr) !=
               APPMODEL_ERROR_NO_PACKAGE;
    }();

    return packaged;

#else

    return false;

#endif
}

} // namespace Fernanda::Msix
