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

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QPdfDocument>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/FileTypes.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

class PdfFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit PdfFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : AbstractFileModel(FileTypes::Pdf, path, parent)
    {
        setup_();
    }

    virtual ~PdfFileModel() override { TRACER; }

    QPdfDocument* document() const noexcept { return document_; }
    virtual QByteArray data() const override { return data_; }

    virtual void setData(const QByteArray& data) override
    {
        data_ = data;

        buffer_.close();
        buffer_.setData(data_);
        buffer_.open(QIODevice::ReadOnly);

        document_->load(&buffer_);
    }

private:
    QByteArray data_{};
    QBuffer buffer_{};
    QPdfDocument* document_ = new QPdfDocument(this);

    void setup_()
    {
        connect(
            document_,
            &QPdfDocument::statusChanged,
            this,
            [this](QPdfDocument::Status status) {
                if (status == QPdfDocument::Status::Error)
                    WARN("PDF load failed for [{}]", meta()->path());
            });
    }
};

} // namespace Fernanda
