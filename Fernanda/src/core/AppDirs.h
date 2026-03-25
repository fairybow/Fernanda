/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Version.h"

// For creating and retrieving non-configurable application paths
// (Coco::Path::SystemDir won't work as a static member variable, since it'll be
// initialized before Qt has been initialized)
//
// TODO: Create these on demand?
namespace Fernanda::AppDirs {

// clang-format off
//
// Structure:
//
// ~/.fernanda/
// |-- ~notebooks/
// |-- ~recovery/
// |   |-- notebooks/
// |   +-- notepad/
// |-- backups/
// |   |-- notebooks/
// |   +-- notepad/
// +-- themes/
// 
// ~/Documents/Fernanda/
//
// clang-format on

namespace Internal {

    inline const Coco::Path& ensured(const Coco::Path& dir)
    {
        if (!dir.exists() && !Coco::mkpath(dir)) {
            CRITICAL("AppDirs: failed to create directory: {}", dir);
        }

        return dir;
    }

} // namespace Internal

inline const Coco::Path& userData()
{
    static Coco::Path dir = Internal::ensured(Coco::Path::Home(".fernanda"));
    return dir;
}

inline const Coco::Path& tempNotebooks()
{
    static Coco::Path dir = Internal::ensured(userData() / "~notebooks");
    return dir;
}

inline const Coco::Path& tempRecovery()
{
    static Coco::Path dir = Internal::ensured(userData() / "~recovery");
    return dir;
}

inline const Coco::Path& tempNotebookRecovery()
{
    static Coco::Path dir = Internal::ensured(tempRecovery() / "notebooks");
    return dir;
}

inline const Coco::Path& tempNotepadRecovery()
{
    static Coco::Path dir = Internal::ensured(tempRecovery() / "notepad");
    return dir;
}

inline const Coco::Path& backups()
{
    static Coco::Path dir = Internal::ensured(userData() / "backups");
    return dir;
}

inline const Coco::Path& notebookBackups()
{
    static Coco::Path dir = Internal::ensured(backups() / "notebooks");
    return dir;
}

inline const Coco::Path& notepadBackups()
{
    static Coco::Path dir = Internal::ensured(backups() / "notepad");
    return dir;
}

inline const Coco::Path& themes()
{
    static Coco::Path dir = Internal::ensured(userData() / "themes");
    return dir;
}

// TODO: Make configurable? Keep always?
inline const Coco::Path& defaultDocs()
{
    static Coco::Path dir =
        Internal::ensured(Coco::Path::Documents(VERSION_APP_NAME_STRING));
    return dir;
}

// Deletes the temp and recovery directories only!
// TODO: Log failure before quit?
inline void cleanup()
{
    for (auto& dir : { tempNotepadRecovery(),
                       tempNotebookRecovery(),
                       tempRecovery(),
                       tempNotebooks() }) {
        Coco::rmdir(dir); // Fails if the dir isn't empty
    }
}

} // namespace Fernanda::AppDirs
