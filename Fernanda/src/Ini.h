/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>

#include "Coco/Path.h"

namespace Fernanda::Ini {

namespace Keys {

    constexpr auto FONT = "Editor/Font";
    constexpr auto EDITOR_THEME = "Editor/Theme";

} // namespace Keys

namespace Defaults {

    constexpr auto FONT_SIZE_MIN = 8;
    constexpr auto FONT_SIZE_MAX = 144;

    inline QFont font()
    {
        QFont f("mononoki", 14);
        f.setBold(false);
        f.setItalic(false);
        return f;
    }

    inline Coco::Path editorTheme()
    {
        // TODO: Go with generic "light" theme (or no theme (empty path)) later
        //return ":/themes/Snooze.fernanda_editor";
        return {}; // testing
    }

} // namespace Defaults

} // namespace Fernanda::Ini
