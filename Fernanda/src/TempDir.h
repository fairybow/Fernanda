/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QString>
#include <QTemporaryDir>

#include "Coco/Path.h"

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
