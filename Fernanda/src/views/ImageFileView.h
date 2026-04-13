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
#include <QByteArray>
#include <QIODevice>
#include <QMovie>
#include <QPixmap>
#include <QShowEvent>
#include <QWidget>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "models/FileMeta.h"
#include "models/RawFileModel.h"
#include "ui/ZoomControl.h"
#include "ui/ZoomState.h"
#include "views/AbstractFileView.h"
#include "views/ImageGraphicsView.h"

namespace Fernanda {

// TODO: Support SVG
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

            movie_ = new QMovie(movieBuffer_, {}, this);
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
            [this](ZoomState::Step s) {
                zoom_.step(s);
                applyZoom_();
            });

        connect(zoomControl_, &ZoomControl::toggleModeRequested, this, [this] {
            zoom_.toggleMode();
            applyZoom_();
        });

        connect(zoomControl_, &ZoomControl::resetRequested, this, [this] {
            zoom_.reset();
            applyZoom_();
        });

        connect(
            graphicsView_,
            &ImageGraphicsView::wheelZoomRequested,
            this,
            [this](ZoomState::Step s) {
                zoom_.step(s);
                applyZoom_();
            });

        applyZoom_();

        return graphicsView_;
    }

    virtual void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);
        if (zoom_.mode() == ZoomState::Fit) applyZoom_();
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        AbstractFileView::resizeEvent(event);
        if (zoom_.mode() == ZoomState::Fit) applyZoom_();
    }

private:
    ImageGraphicsView* graphicsView_ = new ImageGraphicsView(this);
    ZoomControl* zoomControl_ = new ZoomControl(graphicsView_);
    ZoomState zoom_{};

    QBuffer* movieBuffer_ = nullptr;
    QMovie* movie_ = nullptr;

    void applyZoom_()
    {
        if (zoom_.mode() == ZoomState::Fit) {
            graphicsView_->fitToView();
        } else {
            graphicsView_->zoomToFactor(zoom_.factor());
        }

        zoomControl_->setDisplayText(zoom_.displayText());
    }
};

} // namespace Fernanda
