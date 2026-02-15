/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Tr.h"

#include <QString>

#include "Application.h"
#include "Version.h"

namespace Fernanda {

QString tr(const char* sourceText, const char* disambiguation, int n)
{
    constexpr auto context = "Fernanda::Tr";
    return Application::translate(context, sourceText, disambiguation, n);
}

} // namespace Fernanda
