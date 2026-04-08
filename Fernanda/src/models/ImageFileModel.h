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

#include <QByteArray>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Files.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

class ImageFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit ImageFileModel(
        Files::Type fileType,
        const Coco::Path& path,
        QObject* parent = nullptr)
        : AbstractFileModel(fileType, path, parent)
    {
        setup_();
    }

    virtual ~ImageFileModel() override { TRACER; }

    virtual QByteArray data() const override { return data_; }
    virtual void setData(const QByteArray& data) override { data_ = data; }

private:
    QByteArray data_{};

    void setup_()
    {
        //
    }
};

} // namespace Fernanda
