#pragma once

#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QtGlobal>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Fx.h"

class TabViewUnderlay : public QWidget
{
    Q_OBJECT

public:
    explicit TabViewUnderlay(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        initialize_();
    }

    virtual ~TabViewUnderlay() override { COCO_TRACER; }

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
        auto available_space = qMin(rect.width() - 2 * PIXMAP_PADDING_, rect.height() - 2 * PIXMAP_PADDING_);
        auto image_size = qBound(MIN_PIXMAP_SIZE_, available_space, MAX_PIXMAP_SIZE_);

        QRect image_rect
        (
            (rect.width() - image_size) / 2,
            (rect.height() - image_size) / 2,
            image_size,
            image_size
        );

        auto pixmap = QPixmap(":/icons/Fernanda-128.png"); // This looks fine honestly
        pixmap = Coco::Fx::toGreyscale(pixmap);

        // Draw with opacity to let background show through
        painter.setOpacity(0.4); // Adjust this value (0.0 = invisible, 1.0 = opaque)
        painter.drawPixmap(image_rect, pixmap);
        painter.setOpacity(1.0); // Reset opacity
    }

private:
    static constexpr auto PIXMAP_PADDING_ = 15;
    static constexpr auto MIN_PIXMAP_SIZE_ = 80;
    static constexpr auto MAX_PIXMAP_SIZE_ = 100;

    void initialize_()
    {
        setAttribute(Qt::WA_StyledBackground);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};
