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

#include <QFont>
#include <QHash>
#include <QString>
#include <QTextOption>
#include <QVariant>

#include <Coco/Path.h>
#include <Coco/Utility.h>

#include "ui/ColorBar.h"

namespace Fernanda::Ini {

namespace Keys {

    using namespace Qt::StringLiterals;

    inline const auto EDITOR_FONT = u"Editor/Font"_s;
    inline const auto WINDOW_THEME = u"Window/Theme"_s;
    inline const auto EDITOR_THEME = u"Editor/Theme"_s;
    inline const auto KEY_FILTERS_ACTIVE = u"KeyFilters/Active"_s;
    inline const auto KEY_FILTERS_AUTO_CLOSE = u"KeyFilters/AutoClose"_s;
    inline const auto KEY_FILTERS_BARGING = u"KeyFilters/Barging"_s;
    inline const auto EDITOR_CENTER_ON_SCROLL = u"Editor/CenterOnScroll"_s;
    inline const auto EDITOR_OVERWRITE = u"Editor/Overwrite"_s;
    inline const auto EDITOR_TAB_STOP_DISTANCE = u"Editor/TabStopDistance"_s;
    inline const auto EDITOR_WRAP_MODE = u"Editor/WrapMode"_s;
    inline const auto EDITOR_DBL_CLICK_WHITESPACE =
        u"Editor/DoubleClickWhitespace"_s;
    inline const auto EDITOR_LINE_NUMBERS = u"Editor/LineNumbers"_s;
    inline const auto EDITOR_LINE_HIGHLIGHT = u"Editor/LineHighlight"_s;
    inline const auto EDITOR_SELECTION_HANDLES = u"Editor/SelectionHandles"_s;
    inline const auto WORD_COUNTER_ACTIVE = u"WordCounter/Active"_s;
    inline const auto WORD_COUNTER_LINE_COUNT = u"WordCounter/LineCount"_s;
    inline const auto WORD_COUNTER_WORD_COUNT = u"WordCounter/WordCount"_s;
    inline const auto WORD_COUNTER_CHAR_COUNT = u"WordCounter/CharCount"_s;
    inline const auto WORD_COUNTER_SELECTION = u"WordCounter/Selection"_s;
    inline const auto WORD_COUNTER_SEL_REPLACE =
        u"WordCounter/SelectionReplacement"_s;
    inline const auto WORD_COUNTER_LINE_POS = u"WordCounter/LinePos"_s;
    inline const auto WORD_COUNTER_COL_POS = u"WordCounter/ColPos"_s;
    inline const auto COLOR_BAR_ACTIVE = u"ColorBar/Active"_s;
    inline const auto COLOR_BAR_POSITION = u"ColorBar/Position"_s;

} // namespace Keys

// Per-workspace-type keys that don't cascade through TieredSettings
namespace LocalKeys {

    using namespace Qt::StringLiterals;

    inline const auto NOTEPAD_TREE_VIEW_DOCK = u"Notepad/TreeViewDock"_s;
    inline const auto NOTEBOOK_TREE_VIEW_DOCK = u"Notebook/TreeViewDock"_s;

} // namespace LocalKeys

namespace Limits {

    constexpr auto FONT_SIZE_MIN = 8;
    constexpr auto FONT_SIZE_MAX = 144;
    constexpr auto EDITOR_TAB_STOP_DISTANCE_MIN = 20;
    constexpr auto EDITOR_TAB_STOP_DISTANCE_MAX = 140;

} // namespace Limits

using Map = QHash<QString, QVariant>;

inline const Map& defaults()
{
    static const auto font = [] {
        QFont f("mononoki", 18);
        f.setBold(true);
        return f;
    }();

    static const Map map{
        // Font
        { Keys::EDITOR_FONT, qVar(font) },

        // Themes
        { Keys::WINDOW_THEME, qVar(Coco::Path{}) },
        { Keys::EDITOR_THEME,
          qVar(Coco::Path(":/themes/Pocket.fernanda_editor")) },

        // Key filters
        { Keys::KEY_FILTERS_ACTIVE, true },
        { Keys::KEY_FILTERS_AUTO_CLOSE, true },
        { Keys::KEY_FILTERS_BARGING, true },

        // Editor
        { Keys::EDITOR_CENTER_ON_SCROLL, false },
        { Keys::EDITOR_OVERWRITE, false },
        { Keys::EDITOR_TAB_STOP_DISTANCE, 40 },
        { Keys::EDITOR_WRAP_MODE, qVar(QTextOption::WordWrap) },
        { Keys::EDITOR_DBL_CLICK_WHITESPACE, true },
        { Keys::EDITOR_LINE_NUMBERS, true },
        { Keys::EDITOR_LINE_HIGHLIGHT, true },
        { Keys::EDITOR_SELECTION_HANDLES, true },

        // Word counter
        { Keys::WORD_COUNTER_ACTIVE, false },
        { Keys::WORD_COUNTER_LINE_COUNT, true },
        { Keys::WORD_COUNTER_WORD_COUNT, true },
        { Keys::WORD_COUNTER_CHAR_COUNT, false },
        { Keys::WORD_COUNTER_SELECTION, true },
        { Keys::WORD_COUNTER_SEL_REPLACE, true },
        { Keys::WORD_COUNTER_LINE_POS, true },
        { Keys::WORD_COUNTER_COL_POS, true },

        // Color bar
        { Keys::COLOR_BAR_ACTIVE, true },
        { Keys::COLOR_BAR_POSITION, qVar(ColorBar::Top) },

        // Local (per-Workspace)
        { LocalKeys::NOTEPAD_TREE_VIEW_DOCK, false },
        { LocalKeys::NOTEBOOK_TREE_VIEW_DOCK, true },
    };

    return map;
}

} // namespace Fernanda::Ini
