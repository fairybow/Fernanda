/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSize>
#include <QWheelEvent>
#include <QWidget>

#include "core/Debug.h"
#include "ui/ZoomState.h"

namespace Hearth {

// Pannable, zoomable QGraphicsView for image display. Forwards wheel zoom
// requests to its owner rather than handling zoom state itself
class ImageGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageGraphicsView(QWidget* parent = nullptr)
        : QGraphicsView(parent)
    {
        setup_();
    }

    virtual ~ImageGraphicsView() override { TRACER; }

    void setPixmap(const QPixmap& pixmap)
    {
        pixmapItem_->setPixmap(pixmap);
        scene_->setSceneRect(pixmapItem_->boundingRect());
    }

    void zoomToFactor(qreal factor)
    {
        resetTransform();
        scale(factor, factor);
    }

    void fitToView()
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        if (pixmapItem_->pixmap().isNull()) return;

        // Don't upscale past native resolution
        auto pix_size = pixmapItem_->pixmap().size();
        auto view_size = viewport()->size();

        if (pix_size.width() <= view_size.width()
            && pix_size.height() <= view_size.height()) {
            resetTransform();
        } else {
            fitInView(pixmapItem_, Qt::KeepAspectRatio);
        }
    }

signals:
    void wheelZoomRequested(ZoomState::Step step);

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        auto step =
            (event->angleDelta().y() > 0) ? ZoomState::In : ZoomState::Out;
        emit wheelZoomRequested(step);
        event->accept();
    }

private:
    QGraphicsScene* scene_ = new QGraphicsScene(this);
    QGraphicsPixmapItem* pixmapItem_ =
        new QGraphicsPixmapItem; // Parented by scene_ in setup_()

    void setup_()
    {
        setScene(scene_);
        scene_->addItem(pixmapItem_);

        setBackgroundBrush(Qt::black);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setFrameShape(QFrame::NoFrame);
        setAlignment(Qt::AlignCenter);
    }
};

} // namespace Hearth
