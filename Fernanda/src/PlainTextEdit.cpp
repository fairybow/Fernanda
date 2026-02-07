/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "PlainTextEdit.h"

#include "Application.h"

namespace Fernanda {

void PlainTextEdit::onCursorPositionChanged_()
{
    // Ensuring cursor is visible by just calling `setTextCursor(textCursor())`
    // is not ideal, since though it does work, it doesn't change the
    // application's universal cursor blink timer, meaning the cursor is visible
    // for an inconsistent amount of time after each move. It looks and feels
    // bad! This does not do that.

    static auto original_flash_time = -1;

    if (original_flash_time < 0) {
        original_flash_time = Application::cursorFlashTime();
    }

    Application::setCursorFlashTime(0);
    Application::setCursorFlashTime(original_flash_time);
}

} // namespace Fernanda
