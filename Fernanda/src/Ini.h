/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QString>
#include <QVariant>

#include "Utility.h"

namespace Fernanda::Ini {

//...

namespace Editor {

    /// Move anything to constants?
    constexpr auto FONT_KEY = "Editor/Font";
    constexpr auto FONT_PT_SIZE_MIN = 8;
    constexpr auto FONT_PT_SIZE_MAX = 72;
    constexpr auto DEFAULT_FONT_FAMILY = "mononoki";
    constexpr auto DEFAULT_FONT_PT_SIZE = 14;
    constexpr auto DEFAULT_FONT_BOLD = false;
    constexpr auto DEFAULT_FONT_ITALIC = false;

    inline QVariant defaultFont()
    {
        QFont font{};
        font.setFamily(DEFAULT_FONT_FAMILY);
        font.setPointSize(DEFAULT_FONT_PT_SIZE);
        font.setBold(DEFAULT_FONT_BOLD);
        font.setItalic(DEFAULT_FONT_ITALIC);
        return toQVariant(font);
    }

} // namespace Editor

} // namespace Fernanda::Ini
