/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
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
#include <QTemporaryDir>

#include <Coco/Path.h>

namespace Fernanda {

// Utility class for using QTemporaryDir with Coco::Path
// TODO: Move to Coco
class TempDir : public QTemporaryDir
{
public:
    using QTemporaryDir::QTemporaryDir;

    TempDir(const Coco::Path& templatePath)
        : QTemporaryDir(templatePath.toQString())
    {
    }

    const Coco::Path path() const { return QTemporaryDir::path(); }
};

} // namespace Fernanda
