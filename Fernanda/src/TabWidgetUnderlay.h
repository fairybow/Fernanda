/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "Coco/Fx.h"

#include "Debug.h"

namespace Fernanda {

// Background placeholder widget displayed when TabWidget contains no tabs,
// showing a centered, semi-transparent logo with responsive sizing
class TabWidgetUnderlay : public QWidget
{
    Q_OBJECT

public:
    explicit TabWidgetUnderlay(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TabWidgetUnderlay() override { TRACER; }

    virtual QSize minimumSizeHint() const override
    {
        auto side = MIN_PIXMAP_SIZE_ + (2 * PIXMAP_PADDING_);
        return { side, side };
    }

protected:
    virtual void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Calculate center position for image
        auto rect = this->rect();

        // Calculate responsive image size
        auto available_space = qMin(
            rect.width() - 2 * PIXMAP_PADDING_,
            rect.height() - 2 * PIXMAP_PADDING_);
        auto image_size =
            qBound(MIN_PIXMAP_SIZE_, available_space, MAX_PIXMAP_SIZE_);

        QRect image_rect(
            (rect.width() - image_size) / 2,
            (rect.height() - image_size) / 2,
            image_size,
            image_size);

        auto pixmap =
            QPixmap(":/icons/Fernanda-128.png"); // This looks fine honestly
        pixmap = Coco::Fx::toGreyscale(pixmap);

        // Draw with opacity to let background show through
        painter.setOpacity(0.4);
        painter.drawPixmap(image_rect, pixmap);
        painter.setOpacity(1.0); // Reset
    }

private:
    static constexpr auto PIXMAP_PADDING_ = 15;
    static constexpr auto MIN_PIXMAP_SIZE_ = 80;
    static constexpr auto MAX_PIXMAP_SIZE_ = 100;

    void setup_()
    {
        setAttribute(Qt::WA_StyledBackground);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

} // namespace Fernanda
