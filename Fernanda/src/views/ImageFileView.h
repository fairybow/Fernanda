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
#include "views/ImageGraphicsView.h"

namespace Fernanda {

// TODO: Support SVG
/// TODO IV: Move common zoom-related code from ImageGraphicsView,
/// ImageFileView, and PdfFileView (including Mode_ enum and things that operate
/// with it, probably)
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
        auto raw_model = qobject_cast<RawFileModel*>(model());
        ASSERT(raw_model, "RawFileModel cast failed!");

        auto meta = raw_model->meta();

        if (meta->fileType() == Files::Gif) {
            movieBuffer_ = new QBuffer(this);
            movieBuffer_->setData(raw_model->data());
            movieBuffer_->open(QIODevice::ReadOnly);

            movie_ = new QMovie(movieBuffer_, QByteArray(), this);
            movie_->start();

            connect(movie_, &QMovie::frameChanged, this, [this] {
                graphicsView_->setPixmap(
                    QPixmap::fromImage(movie_->currentImage()));
            });

        } else {
            QPixmap pixmap{};

            if (pixmap.loadFromData(raw_model->data())) {
                graphicsView_->setPixmap(pixmap);
            } else {
                WARN("Image load failed for [{}]", meta->path());
            }
        }

        connect(
            zoomControl_,
            &ZoomControl::stepRequested,
            this,
            [this](int direction) {
                mode_ = Fixed;
                factor_ = qBound(
                    MIN_FACTOR_,
                    factor_ + (STEP_ * direction),
                    MAX_FACTOR_);
                applyZoom_();
            });

        connect(zoomControl_, &ZoomControl::toggleModeRequested, this, [this] {
            mode_ = (mode_ == Fit) ? Fixed : Fit;
            applyZoom_();
        });

        connect(zoomControl_, &ZoomControl::resetRequested, this, [this] {
            mode_ = Fixed;
            factor_ = 1.0;
            applyZoom_();
        });

        connect(
            graphicsView_,
            &ImageGraphicsView::wheelZoomRequested,
            this,
            [this](int steps) {
                mode_ = Fixed;
                factor_ =
                    qBound(MIN_FACTOR_, factor_ + (STEP_ * steps), MAX_FACTOR_);
                applyZoom_();
            });

        applyZoom_();

        return graphicsView_;
    }

    virtual void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);
        if (mode_ == Fit) applyZoom_();
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        AbstractFileView::resizeEvent(event);
        if (mode_ == Fit) applyZoom_();
    }

private:
    enum Mode_
    {
        Fit,
        Fixed
    };

    ImageGraphicsView* graphicsView_ = new ImageGraphicsView(this);
    ZoomControl* zoomControl_ = new ZoomControl(graphicsView_);

    Mode_ mode_ = Fit;
    qreal factor_ = 1.0;

    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;

    QBuffer* movieBuffer_ = nullptr;
    QMovie* movie_ = nullptr;

    void applyZoom_()
    {
        if (mode_ == Fit) {
            graphicsView_->fitToView();
            zoomControl_->setDisplayText(QStringLiteral("Fit"));
        } else {
            graphicsView_->zoomToFactor(factor_);
            zoomControl_->setDisplayText(
                QStringLiteral("%1%").arg(qRound(factor_ * 100.0)));
        }
    }
};

} // namespace Fernanda
