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
namespace Fernanda::AppDirs {

inline const Coco::Path& userData()
{
    static auto dir = Coco::Path::Home(".fernanda");
    return dir;
}

// TODO: Backups folder

inline const Coco::Path& temp()
{
    static auto dir = userData() / "temp";
    return dir;
}

inline const Coco::Path& userThemes()
{
    static auto dir = userData() / "themes";
    return dir;
}

inline const Coco::Path& notepadBackups()
{
    static auto dir = userData() / "backups" / "notepad";
    return dir;
}

inline const Coco::Path& notebookBackups()
{
    static auto dir = userData() / "backups" / "notebooks";
    return dir;
}

// TODO: Make configurable
inline const Coco::Path& defaultDocs()
{
    static auto dir = Coco::Path::Documents(VERSION_APP_NAME_STRING);
    return dir;
}

// Cannot be called before Application has finished construction, since it
// relies on Path::SystemDir functions, which only work once Qt is ready
inline bool initialize()
{
    auto& t = temp();
    auto& th = userThemes();
    auto& d = defaultDocs();

    auto temp_ok = t.exists() || Coco::mkdir(t);
    auto themes_ok = th.exists() || Coco::mkdir(th);
    auto docs_ok = d.exists() || Coco::mkdir(d);

    if (!temp_ok) CRITICAL("Temp directory non-existent!: {}", t);
    if (!themes_ok) CRITICAL("User themes directory non-existent!: {}", th);
    if (!docs_ok) CRITICAL("Docs directory non-existent!: {}", d);

    return temp_ok && themes_ok && docs_ok;
}

} // namespace Fernanda::AppDirs
