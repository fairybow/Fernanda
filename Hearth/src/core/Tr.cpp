/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "core/Tr.h"

#include <QString>

#include "core/Application.h"
#include "core/Version.h"

namespace Fernanda {

QString tr(const char* sourceText, const char* disambiguation, int n)
{
    constexpr auto context = "Fernanda::Tr";
    return Application::translate(context, sourceText, disambiguation, n);
}

} // namespace Fernanda
