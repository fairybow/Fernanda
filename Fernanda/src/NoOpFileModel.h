/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>

#include "Coco/Path.h"

#include "Debug.h"
#include "IFileModel.h"

namespace Fernanda {

// Placeholder file model for potentially non-viewable file types, providing
// basic IFileModel interface without logic
class NoOpFileModel : public IFileModel
{
    Q_OBJECT

public:
    explicit NoOpFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : IFileModel(path, parent)
    {
    }

    virtual ~NoOpFileModel() override { TRACER; }
};

} // namespace Fernanda
