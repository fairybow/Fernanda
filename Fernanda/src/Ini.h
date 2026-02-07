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
#include <QString>

// TODO: Any way to get path to work with QSettings?
// #include "Coco/Path.h"

namespace Fernanda::Ini {

namespace Keys {

    constexpr auto EDITOR_FONT = "Editor/Font";
    constexpr auto WINDOW_THEME = "Window/Theme";
    constexpr auto EDITOR_THEME = "Editor/Theme";
    constexpr auto TREE_VIEW = "Window/TreeView";

} // namespace Keys

namespace Defaults {

    constexpr auto FONT_SIZE_MIN = 8;
    constexpr auto FONT_SIZE_MAX = 144;

    inline QFont font()
    {
        QFont f("mononoki", 18);
        f.setBold(true);
        f.setItalic(false);
        return f;
    }

    inline QString windowTheme() { return {}; }

    // TODO: Any way to get Coco::Path to work with QSettings?
    inline QString editorTheme() { return ":/themes/Pocket.fernanda_editor"; }

    /// TODO TVT: Notepad should default to false and Notebooks should default
    /// to true. How to do this?

} // namespace Defaults

} // namespace Fernanda::Ini
