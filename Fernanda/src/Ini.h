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
#include <QTextOption>

// TODO: Any way to get path to work with QSettings?
// #include "Coco/Path.h"

namespace Fernanda::Ini {

namespace Keys {

    constexpr auto EDITOR_FONT = "Editor/Font";
    constexpr auto WINDOW_THEME = "Window/Theme";
    constexpr auto EDITOR_THEME = "Editor/Theme";
    // Non-cascading
    constexpr auto NOTEPAD_TREE_VIEW_DOCK = "Notepad/TreeViewDock";
    // Non-cascading
    constexpr auto NOTEBOOK_TREE_VIEW_DOCK = "Notebook/TreeViewDock";
    constexpr auto KEY_FILTERS_ACTIVE = "KeyFilters/Active";
    constexpr auto KEY_FILTERS_AUTO_CLOSE = "KeyFilters/AutoClose";
    constexpr auto KEY_FILTERS_BARGING = "KeyFilters/Barging";
    constexpr auto EDITOR_CENTER_ON_SCROLL = "Editor/CenterOnScroll";
    constexpr auto EDITOR_OVERWRITE = "Editor/Overwrite";
    constexpr auto EDITOR_TAB_STOP_DISTANCE = "Editor/TabStopDistance";
    constexpr auto EDITOR_WORD_WRAP_MODE = "Editor/WordWrapMode";

} // namespace Keys

namespace Defaults {

    constexpr auto FONT_SIZE_MIN = 8;
    constexpr auto FONT_SIZE_MAX = 144;
    constexpr auto EDITOR_TAB_STOP_DISTANCE_MIN = 20;
    constexpr auto EDITOR_TAB_STOP_DISTANCE_MAX = 140;

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
    inline bool notepadTreeViewDock() { return false; }
    inline bool notebookTreeViewDock() { return true; }
    inline bool keyFiltersActive() { return true; }
    inline bool keyFiltersAutoClose() { return true; }
    inline bool keyFiltersBarging() { return true; }
    inline bool editorCenterOnScroll() { return false; }
    inline bool editorOverwrite() { return false; }
    inline int editorTabStopDistance() { return 40; }
    inline QTextOption::WrapMode editorWordWrapMode()
    {
        return QTextOption::WordWrap;
    }

} // namespace Defaults

} // namespace Fernanda::Ini
