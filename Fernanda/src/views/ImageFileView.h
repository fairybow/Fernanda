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

#include <QBuffer>
#include <QColor>
#include <QLabel>
#include <QMovie>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QSize>
#include <QWidget>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "models/AbstractFileModel.h"
#include "models/FileMeta.h"
#include "models/RawFileModel.h"
#include "ui/ZoomControl.h"
#include "views/AbstractFileView.h"

namespace Fernanda {

// TODO: Support SVG (unsure how to detect atm; may need SVG widgets component,
// in which case can probably drop plain svg component and it'll be brought in
// anyway)
// TODO: Scroll bars are all black. Impossible to set them back to original
// color. Setting black palette on scrollArea_->viewport() instead of
// scrollArea_ does nothing (white background)
class ImageFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit ImageFileView(RawFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~ImageFileView() override { TRACER; }

    virtual bool isUserEditable() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        scrollArea_->setWidgetResizable(false);
        scrollArea_->setAlignment(Qt::AlignCenter);
        setPalette_(scrollArea_, Qt::black);

        label_->setScaledContents(false);
        label_->setAlignment(Qt::AlignCenter);
        setPalette_(label_, Qt::transparent);

        auto raw_model = qobject_cast<RawFileModel*>(model());
        ASSERT(raw_model, "RawFileModel cast failed!");

        auto meta = raw_model->meta();

        if (meta->fileType() == Files::Gif) {
            movieBuffer_ = new QBuffer(this);
            movieBuffer_->setData(raw_model->data());
            movieBuffer_->open(QIODevice::ReadOnly);

            movie_ = new QMovie(movieBuffer_, QByteArray(), this);
            label_->setMovie(movie_);
            movie_->start();
            originalMovieSize_ = movie_->currentImage().size();

            // WARN for failure here, too? (See below)

        } else {
            QPixmap pixmap{};

            if (pixmap.loadFromData(raw_model->data())) {
                originalPixmap_ = pixmap; // Show event resizes for us
            } else {
                WARN("Image load failed for [{}]", meta->path());
            }
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
    ZoomControl* zoomControl_ = new ZoomControl(scrollArea_);
    QPixmap originalPixmap_{};

    // Gif:
    QBuffer* movieBuffer_ = nullptr;
    QMovie* movie_ = nullptr;
    QSize originalMovieSize_{};

    // TODO: Perhaps temp, perhaps not. May never want to honor QSS here (keep
    // black)
    void setPalette_(QWidget* widget, const QColor& color)
    {
        widget->setAutoFillBackground(true);
        widget->setBackgroundRole(QPalette::Base); // Not technically necessary
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Base, color);
        widget->setPalette(palette);
    }

    void zoomToFactor_(qreal factor)
    {
        // Gif:
        if (movie_) {
            auto scaled = originalMovieSize_ * factor;
            movie_->setScaledSize(scaled);
            label_->resize(scaled);
            return;
        }

        // Everything else:
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
        // Gif:
        if (movie_) {
            auto scaled = originalMovieSize_.scaled(
                originalMovieSize_.boundedTo(
                    scrollArea_->maximumViewportSize()),
                Qt::KeepAspectRatio);
            movie_->setScaledSize(scaled);
            label_->resize(scaled);
            return;
        }

        // Everything else:
        if (originalPixmap_.isNull()) return;

        // Don't exceed resolution
        auto scaled = originalPixmap_.scaled(
            originalPixmap_.size().boundedTo(
                scrollArea_->maximumViewportSize()),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        label_->setPixmap(scaled);
        label_->resize(scaled.size());
    }
};

} // namespace Fernanda
