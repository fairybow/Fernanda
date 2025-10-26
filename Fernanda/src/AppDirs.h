/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Debug.h"
#include "Version.h"

// For creating and retrieving application paths
namespace Fernanda::AppDirs {

inline const Coco::Path& userData()
{
    // Coco::Path::SystemDir won't work as a static variable, since it'll be
    // initialized before Qt has been initialized
    static auto dir = Coco::Path::Home(".fernanda");
    return dir;
}

inline const Coco::Path& temp()
{
    static auto dir = userData() / "temp";
    return dir;
}

inline const Coco::Path& defaultDocs()
{
    static auto dir = Coco::Path::Documents(VERSION_APP_NAME_STRING);
    return dir;
}

inline bool initialize()
{
    auto& t = temp();
    auto& d = defaultDocs();

    auto temp_ok = t.exists() || Coco::PathUtil::mkdir(t);
    auto docs_ok = d.exists() || Coco::PathUtil::mkdir(d);

    if (!temp_ok) CRITICAL("Temp directory non-existent!: {}", t);
    if (!docs_ok) CRITICAL("Docs directory non-existent!: {}", d);

    return temp_ok && docs_ok;
}

} // namespace Fernanda::AppDirs
