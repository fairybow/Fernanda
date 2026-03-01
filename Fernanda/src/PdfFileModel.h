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

class PdfFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit PdfFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : AbstractFileModel(path, parent)
    {
        setup_();
    }

    virtual ~PdfFileModel() override { TRACER; }

    QPdfDocument* document() const noexcept { return document_; }

    virtual QByteArray data() const override { return rawData_; }
    virtual bool supportsModification() const override { return false; }

    virtual void setData(const QByteArray& data) override
    {
        rawData_ = data;

        buffer_.close();
        buffer_.setData(rawData_);
        buffer_.open(QIODevice::ReadOnly);

        document_->load(&buffer_);
    }

private:
    QByteArray rawData_{};
    QBuffer buffer_{};
    QPdfDocument* document_ = new QPdfDocument(this);

    void setup_()
    {
        connect(
            document_,
            &QPdfDocument::statusChanged,
            this,
            [&](QPdfDocument::Status status) {
                if (status == QPdfDocument::Status::Error)
                    WARN("PDF load failed for [{}]", meta()->path());
            });
    }
};

} // namespace Fernanda
