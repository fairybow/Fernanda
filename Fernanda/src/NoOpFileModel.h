#pragma once

#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

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

    virtual ~NoOpFileModel() override { COCO_TRACER; }
};

} // namespace Fernanda
