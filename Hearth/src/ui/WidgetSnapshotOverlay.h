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

#include <utility>

#include <QEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

// Captures a pixmap snapshot of a target widget and displays it as a floating
// overlay, positioned via global coordinate mapping. Used to freeze visual
// state during transitions to avoid flicker. Must be constructed as a child of
// the target widget's parent for positioning
class WidgetSnapshotOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSnapshotOverlay(QWidget* parent)
        : QWidget(parent)
    {
        ASSERT(parent);
        setup_();
    }

    virtual ~WidgetSnapshotOverlay() override { TRACER; }

    void captureAndShow(QWidget* target)
    {
        if (!target) return;

        // Only half of the "is available" property. The rest is taken care of
        // by isVisible
        capturing_ = true;

        auto dpr = target->devicePixelRatioF();
        auto size = target->size() * dpr;

        if (size.isEmpty()) return;

        QPixmap pixmap(size);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        target->render(&painter);
        painter.end();

        pixmap_ = std::move(pixmap);
        targetWidget_ = target;
        target->installEventFilter(this);

        syncGeometry_();
        geometryDirty_ = false;

        show();
        raise();

        capturing_ = false;
    }

    void hideOverlay()
    {
        hide();

        if (targetWidget_) {
            targetWidget_->removeEventFilter(this);
            targetWidget_ = nullptr;
        }

        pixmap_ = {};
        geometryDirty_ = false;
    }

    // If used for resizing widgets, may need to avoid reentrancy using this
    // check
    bool isAvailable() const { return !isVisible() && !capturing_; }

protected:
    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        if (pixmap_.isNull()) return;

        QPainter painter(this);

        // Prefer fidelity (no pixmap transform - screengrab is almost visually
        // indistinguishable from the real widgets), but if the geometry is
        // dirty, then we'll prioritize functionality (covering the seams)
        if (geometryDirty_) {
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawPixmap(rect(), pixmap_);
        } else {
            painter.drawPixmap(0, 0, pixmap_);
        }
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != targetWidget_) return false;

        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
            geometryDirty_ = true;
            syncGeometry_();
            break;

        default:
            break;
        }

        return false;
    }

private:
    bool capturing_ = false;
    bool geometryDirty_ = false;
    QPixmap pixmap_{};
    QWidget* targetWidget_ = nullptr;

    void setup_()
    {
        setWindowFlags(Qt::Widget);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        hide();
    }

    void syncGeometry_()
    {
        if (!targetWidget_) return;

        auto global_pos = targetWidget_->mapToGlobal(QPoint(0, 0));
        auto local_pos = parentWidget()->mapFromGlobal(global_pos);

        move(local_pos);
        setFixedSize(targetWidget_->size());
    }
};

} // namespace Fernanda
