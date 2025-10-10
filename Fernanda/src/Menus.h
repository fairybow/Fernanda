/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include "Coco/Bool.h"

#include "Bus.h"
#include "MenuActions.h"
#include "Window.h"

// Utility functions for Notepad and Notebook menu modules
namespace Fernanda::Menus {

COCO_BOOL(AutoRepeat);

// TODO: Ensure we pass -1 to certain commands as arg (for "current editor"
// ops)
inline void
initializeCommonActions(Bus* bus, Window* window, CommonMenuActions& actions)
{
    if (!bus || !window) return;

    //...
}

} // namespace Fernanda::Menus
