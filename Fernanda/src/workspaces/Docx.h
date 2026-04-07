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

#include <QString>
#include <QXmlStreamReader>

#include <miniz.h>

#include <Coco/Path.h>

#include "core/MagicBytes.h"

namespace Fernanda::Docx {

using namespace Qt::StringLiterals;

inline bool isDocxFile(const Coco::Path& path)
{
    return path.ext() == u".docx"_s && MagicBytes::is(MagicBytes::Zip, path);
}

inline QString toPlainText(const Coco::Path& path)
{
    //...
}

} // namespace Fernanda::Docx
