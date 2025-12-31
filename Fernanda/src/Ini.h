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

namespace Fernanda::Ini {

namespace Keys {

    constexpr auto FONT = "Editor/Font";

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

} // namespace Defaults

} // namespace Fernanda::Ini
