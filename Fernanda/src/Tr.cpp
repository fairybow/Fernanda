/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QString>

#include "Application.h"
#include "Tr.h"
#include "Version.h"

namespace Fernanda {

QString tr(const char* sourceText, const char* disambiguation, int n)
{
    return Application::translate(
        VERSION_APP_NAME_STRING,
        sourceText,
        disambiguation,
        n);
}

} // namespace Fernanda
