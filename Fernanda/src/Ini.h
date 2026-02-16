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

#include "ColorBar.h"

// TODO: Any way to get path to work with QSettings?
// #include "Coco/Path.h"

namespace Fernanda::Ini {

namespace Keys {

    constexpr auto EDITOR_FONT = "Editor/Font";
    constexpr auto WINDOW_THEME = "Window/Theme";
    constexpr auto EDITOR_THEME = "Editor/Theme";
    constexpr auto NOTEPAD_TREE_VIEW_DOCK =
        "Notepad/TreeViewDock"; // Non-cascading
    constexpr auto NOTEBOOK_TREE_VIEW_DOCK =
        "Notebook/TreeViewDock"; // Non-cascading
    constexpr auto KEY_FILTERS_ACTIVE = "KeyFilters/Active";
    constexpr auto KEY_FILTERS_AUTO_CLOSE = "KeyFilters/AutoClose";
    constexpr auto KEY_FILTERS_BARGING = "KeyFilters/Barging";
    constexpr auto EDITOR_CENTER_ON_SCROLL = "Editor/CenterOnScroll";
    constexpr auto EDITOR_OVERWRITE = "Editor/Overwrite";
    constexpr auto EDITOR_TAB_STOP_DISTANCE = "Editor/TabStopDistance";
    constexpr auto EDITOR_WRAP_MODE = "Editor/WrapMode";
    constexpr auto EDITOR_DBL_CLICK_WHITESPACE = "Editor/DoubleClickWhitespace";
    constexpr auto EDITOR_LINE_NUMBERS = "Editor/LineNumbers";
    constexpr auto EDITOR_LINE_HIGHLIGHT = "Editor/LineHighlight";
    constexpr auto EDITOR_SELECTION_HANDLES = "Editor/SelectionHandles";
    constexpr auto WORD_COUNTER_ACTIVE = "WordCounter/Active";
    constexpr auto WORD_COUNTER_LINE_COUNT = "WordCounter/LineCount";
    constexpr auto WORD_COUNTER_WORD_COUNT = "WordCounter/WordCount";
    constexpr auto WORD_COUNTER_CHAR_COUNT = "WordCounter/CharCount";
    constexpr auto WORD_COUNTER_SELECTION = "WordCounter/Selection";
    constexpr auto WORD_COUNTER_SEL_REPLACE =
        "WordCounter/SelectionReplacement";
    constexpr auto WORD_COUNTER_LINE_POS = "WordCounter/LinePos";
    constexpr auto WORD_COUNTER_COL_POS = "WordCounter/ColPos";
    constexpr auto COLOR_BAR_ACTIVE = "ColorBar/Active";
    constexpr auto COLOR_BAR_POSITION = "ColorBar/Position";

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
    inline QTextOption::WrapMode editorWrapMode()
    {
        return QTextOption::WordWrap;
    }
    inline bool editorDoubleClickWhitespace() { return true; }
    inline bool editorLineNumbers() { return true; }
    inline bool editorLineHighlight() { return true; }
    inline bool editorSelectionHandles() { return true; }
    inline bool wordCounterActive() { return true; }
    inline bool wordCounterLineCount() { return true; }
    inline bool wordCounterWordCount() { return true; }
    inline bool wordCounterCharCount() { return false; }
    inline bool wordCounterSelection() { return true; }
    inline bool wordCounterSelReplace() { return true; }
    inline bool wordCounterLinePos() { return true; }
    inline bool wordCounterColPos() { return true; }
    inline bool colorBarActive() { return true; }
    inline ColorBar::Position colorBarPosition() { return ColorBar::Top; }

} // namespace Defaults

} // namespace Fernanda::Ini
