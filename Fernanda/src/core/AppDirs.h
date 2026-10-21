/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <QString>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Publication.h"

// For creating and retrieving non-configurable application paths
// (NB: Coco::Path::SystemDir methods won't work as static member variables,
// since they'll be initialized before Qt has been initialized, which will fail)
namespace Fernanda::AppDirs {

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

// Where the create/open dialogs start before any vault has been made there
// (AppConfig::newVaultMruDir), and where the first run makes the Common Vault
// (App::initCommonVault_). Each is made the first time it's asked for
GEN_DIR_METHOD_(defaultDocs, Coco::Path::Documents(PUB_APP_NAME_STRING))
GEN_DIR_METHOD_(defaultCommonVault, defaultDocs() / "Common Vault")

// user/AppData/Local/Fernanda on Windows
GEN_DIR_METHOD_(appData, Coco::Path::GenericData(PUB_APP_NAME_STRING))
GEN_DIR_METHOD_(logs, appData() / "logs")

// Spelling dictionaries (core/spell/SpellCheckers.h): the bundled ones are
// copied here, and any others placed here are found too
GEN_DIR_METHOD_(dictionaries, appData() / "dictionaries")

#undef GEN_DIR_METHOD_

} // namespace Fernanda::AppDirs
