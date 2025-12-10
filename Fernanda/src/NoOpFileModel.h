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

#include "AbstractFileModel.h"
#include "Debug.h"

namespace Fernanda {

class NoOpFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit NoOpFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : AbstractFileModel(path, parent)
    {
    }

    virtual ~NoOpFileModel() override { TRACER; }

    virtual QByteArray data() const override { return {}; }
    virtual bool supportsModification() const override { return false; }
};

} // namespace Fernanda
