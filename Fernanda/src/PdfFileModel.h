/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QPdfDocument>

#include "Coco/Path.h"

#include "AbstractFileModel.h"
#include "Debug.h"

namespace Fernanda {

// TODO: PDF Document
class PdfFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit PdfFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : AbstractFileModel(path, parent)
    {
    }

    virtual ~PdfFileModel() override { TRACER; }

    // TODO: PDF Document
    virtual QByteArray data() const override { return {}; }
    virtual bool supportsModification() const override { return false; }
};

} // namespace Fernanda
