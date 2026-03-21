/*
 * Fernanda is a plain text editor for fiction writing
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
// |-- ~temp/
// |   |-- notebooks/
// |   +-- recovery/
// |       |-- notebooks/
// |       +-- notepad/
// |-- backups/
// |   |-- notebooks/
// |   +-- notepad/
// +-- themes/
// 
// ~/Documents/Fernanda
//
// clang-format on

inline const Coco::Path& userData()
{
    static auto dir = Coco::Path::Home(".fernanda");
    return dir;
}

inline const Coco::Path& temp()
{
    static auto dir = userData() / "~temp";
    return dir;
}

inline const Coco::Path& tempNotebooks()
{
    static auto dir = temp() / "notebooks";
    return dir;
}

inline const Coco::Path& tempRecovery()
{
    static auto dir = temp() / "recovery";
    return dir;
}

inline const Coco::Path& tempNotebookRecovery()
{
    static auto dir = tempRecovery() / "notebooks";
    return dir;
}

inline const Coco::Path& tempNotepadRecovery()
{
    static auto dir = tempRecovery() / "notepad";
    return dir;
}

inline const Coco::Path& backups()
{
    static auto dir = userData() / "backups";
    return dir;
}

inline const Coco::Path& notebookBackups()
{
    static auto dir = backups() / "notebooks";
    return dir;
}

inline const Coco::Path& notepadBackups()
{
    static auto dir = backups() / "notepad";
    return dir;
}

inline const Coco::Path& themes()
{
    static auto dir = userData() / "themes";
    return dir;
}

// TODO: Make configurable
inline const Coco::Path& defaultDocs()
{
    static auto dir = Coco::Path::Documents(VERSION_APP_NAME_STRING);
    return dir;
}

// Cannot be called before Application has finished construction, since it
// relies on Path's system directory functions which only work once Qt is ready
inline bool initialize()
{
    auto all_ok = true;

    for (auto& dir : {
             // Use leaf directories
             tempNotebooks(),
             tempNotebookRecovery(),
             tempNotepadRecovery(),
             notebookBackups(),
             notepadBackups(),
             themes(),
             defaultDocs(),
         }) {
        if (!dir.exists() && !Coco::mkpath(dir)) {
            CRITICAL("AppDirs directory non-existent!: {}", dir);
            all_ok = false;
        }
    }

    return all_ok;
}

// Deletes the temp and recovery directories only!
// TODO: Log failure before quit?
inline void cleanup()
{
    for (auto& dir : { tempNotepadRecovery(),
                       tempNotebookRecovery(),
                       tempRecovery(),
                       tempNotebooks(),
                       temp() }) {
        Coco::rmdir(dir); // Fails if the dir isn't empty
    }
}

} // namespace Fernanda::AppDirs
