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

#include <utility>

#include <QEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QWidget>

#include "core/Debug.h"

/// TODO MU: Untested!

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

    ~WidgetSnapshotOverlay() override { TRACER; }

    void captureAndShow(QWidget* target)
    {
        if (!target) return;

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
        show();
        raise();
    }

    void hideOverlay()
    {
        hide();

        if (targetWidget_) {
            targetWidget_->removeEventFilter(this);
            targetWidget_ = nullptr;
        }

        pixmap_ = {};
    }

protected:
    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        if (pixmap_.isNull()) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(rect(), pixmap_);
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != targetWidget_) return false;

        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
            syncGeometry_();
            break;

        default:
            break;
        }

        return false;
    }

private:
    QWidget* targetWidget_ = nullptr;
    QPixmap pixmap_{};

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
