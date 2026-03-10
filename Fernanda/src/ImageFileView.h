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
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QSize>
#include <QWidget>

#include "Coco/Path.h"

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Debug.h"
#include "FileMeta.h"
#include "ImageFileModel.h"
#include "ZoomControl.h"

namespace Fernanda {

// TODO: Support SVG (unsure how to detect atm; may need SVG widgets component,
// in which case can probably drop plain svg component and it'll be brought in
// anyway)
// TODO: Need zoom controls! (Shows percent, ofc)
// TODO: For this overlay widget, look to SelectionHandleOverlay.h as an example
// Requires Qt Image Formats
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
        scrollArea_->setWidgetResizable(false);
        scrollArea_->setAlignment(Qt::AlignCenter);
        scrollArea_->setAutoFillBackground(true);
        scrollArea_->setBackgroundRole(
            QPalette::Base); // Not technically necessary
        QPalette palette = scrollArea_->palette();
        palette.setColor(QPalette::Base, Qt::black);
        scrollArea_->setPalette(palette);

        label_->setScaledContents(false);
        label_->setAlignment(Qt::AlignCenter);

        if (auto image_model = qobject_cast<ImageFileModel*>(model())) {
            QPixmap pixmap{};

            if (pixmap.loadFromData(image_model->data()))
                originalPixmap_ = pixmap; // Show event resizes for us
            else
                WARN("Image load failed for [{}]", image_model->meta()->path());

        } else {
            FATAL("ImageFileModel cast failed!");
        }

        scrollArea_->setWidget(label_);

        connect(
            zoomControl_,
            &ZoomControl::zoomChanged,
            this,
            [this](ZoomControl::Mode mode, qreal factor) {
                if (mode == ZoomControl::Fit)
                    fitToView_();
                else
                    zoomToFactor_(factor);
            });

        return scrollArea_;
    }

    virtual void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);
        if (zoomControl_->mode() == ZoomControl::Fit) fitToView_();
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        AbstractFileView::resizeEvent(event);
        if (zoomControl_->mode() == ZoomControl::Fit) fitToView_();
    }

private:
    QScrollArea* scrollArea_ = new QScrollArea(this);
    QLabel* label_ = new QLabel(this);
    QPixmap originalPixmap_{};
    ZoomControl* zoomControl_ = new ZoomControl(scrollArea_);

    void zoomToFactor_(qreal factor)
    {
        if (originalPixmap_.isNull()) return;

        auto scaled = originalPixmap_.scaled(
            originalPixmap_.size() * factor,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        label_->setPixmap(scaled);
        label_->resize(scaled.size());
    }

    void fitToView_()
    {
        if (originalPixmap_.isNull()) return;

        // Don't exceed resolution
        auto scaled = originalPixmap_.scaled(
            originalPixmap_.size().boundedTo(scrollArea_->viewport()->size()),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        label_->setPixmap(scaled);
        label_->resize(scaled.size());
    }
};

} // namespace Fernanda
