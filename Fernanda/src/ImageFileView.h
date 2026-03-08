/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QWidget>

#include "Coco/Path.h"

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Debug.h"
#include "FileMeta.h"
#include "ImageFileModel.h"

namespace Fernanda {

// TODO: Need zoom controls!
// TODO: For this overlay widget, look to SelectionHandleOverlay.h as an example
class ImageFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit ImageFileView(ImageFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~ImageFileView() override { TRACER; }

    virtual bool supportsEditing() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        // TODO: How to start with image fit to view size? (Will be able to
        // zoomed later when we make the controls)

        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignCenter);

        label_->setScaledContents(true);
        label_->setAlignment(Qt::AlignCenter);

        if (auto image_model = qobject_cast<ImageFileModel*>(model())) {
            QPixmap pixmap{};

            if (pixmap.loadFromData(image_model->data()))
                label_->setPixmap(pixmap);
            else
                WARN("Image load failed for [{}]", image_model->meta()->path());

        } else {
            FATAL("ImageFileModel cast failed!");
        }

        scrollArea_->setWidget(label_);
        return scrollArea_;
    }

private:
    QScrollArea* scrollArea_ = new QScrollArea(this);
    QLabel* label_ = new QLabel(this);
};

} // namespace Fernanda
