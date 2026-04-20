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
// |-- logs/
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

#define GEN_DIR_METHOD_(Name, Path_)                                           \
    inline const Coco::Path& Name()                                            \
    {                                                                          \
        static Coco::Path dir = Internal::ensured(Path_);                      \
        return dir;                                                            \
    }

GEN_DIR_METHOD_(userData, Coco::Path::Home(".fernanda"))
GEN_DIR_METHOD_(tempNotebooks, userData() / "~notebooks")
GEN_DIR_METHOD_(tempRecovery, userData() / "~recovery")
GEN_DIR_METHOD_(tempNotebookRecovery, tempRecovery() / "notebooks")
GEN_DIR_METHOD_(tempNotepadRecovery, tempRecovery() / "notepad")
GEN_DIR_METHOD_(backups, userData() / "backups")
GEN_DIR_METHOD_(notebookBackups, backups() / "notebooks")
GEN_DIR_METHOD_(notepadBackups, backups() / "notepad")
GEN_DIR_METHOD_(logs, userData() / "logs")
GEN_DIR_METHOD_(themes, userData() / "themes")

// TODO: Make configurable? Keep always?
GEN_DIR_METHOD_(defaultDocs, Coco::Path::Documents(VERSION_APP_NAME_STRING))

#undef GEN_DIR_METHOD_

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
