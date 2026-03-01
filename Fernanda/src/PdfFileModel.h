/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
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

    QPdfDocument* document() const noexcept { return document_; }

    // TODO: PDF Document
    virtual QByteArray data() const override { return rawData_; }
    virtual bool supportsModification() const override { return false; }

    virtual void setData(const QByteArray& data) override
    {
        rawData_ = data;

        // QPdfDocument can load from a QIODevice, so wrap the bytes
        buffer_.close();
        buffer_.setData(rawData_);
        buffer_.open(QIODevice::ReadOnly);
        document_->load(&buffer_);

        // TODO: The load overload for QIODevice doesn't return an error...
        // auto error = document_->load(&buffer_);

        // if (error != QPdfDocument::Error::None)
        //     WARN("Failed to load PDF (error: {})", static_cast<int>(error));
    }

private:
    QPdfDocument* document_ = new QPdfDocument(this);
    QByteArray rawData_{};
    QBuffer buffer_{};
};

} // namespace Fernanda
