/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

#include "core/Debug.h"
#include "ui/Painting.h"

namespace Fernanda {

class TabWidgetAlertWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabWidgetAlertWidget(
        const QString& message,
        QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(message);
    }

    virtual ~TabWidgetAlertWidget() override { TRACER; }

private:
    static constexpr auto ICON_SIZE_ = QSize(16, 16);
    QLabel* label_ = new QLabel(this);

    void setup_(const QString& message)
    {
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(label_);

        buildIcon_();
        setToolTip(message);
    }

    // TODO: StyleContext support
    // TODO: What else can move to Painting (shared with ControlField info
    // icon)?
    void buildIcon_()
    {
        auto total = qMin(ICON_SIZE_.width(), ICON_SIZE_.height());
        auto padding = total / 10.0;
        auto stroke = total / 12.0;

        QPixmap icon(total, total);
        icon.fill(Qt::transparent);

        QPainter painter(&icon);
        painter.setRenderHint(QPainter::Antialiasing);

        // Triangle vertices (pointing up)
        auto left = padding;
        auto right = total - padding;
        auto top = padding;
        auto bottom = total - padding;
        auto center_x = total / 2.0;

        QPainterPath path{};
        path.moveTo(center_x, top);
        path.lineTo(right, bottom);
        path.lineTo(left, bottom);
        path.closeSubpath();

        painter.setPen(QPen(
            Qt::black,
            stroke,
            Qt::SolidLine,
            Qt::RoundCap,
            Qt::RoundJoin));
        painter.setBrush(QColor(255, 210, 0)); // warning yellow
        painter.drawPath(path);

        // "!" glyph
        QFont font = painter.font();
        font.setPixelSize(qRound(total * 0.55));
        font.setBold(true);
        font.setFamily("Arial");
        font.setStyleHint(QFont::SansSerif);
        painter.setFont(font);

        painter.setPen(Qt::black);

        // Center the "!" in the lower 2/3 of the triangle (where it's wider)
        auto glyph_rect = QRectF(
            left,
            top + (bottom - top) * 0.3,
            right - left,
            (bottom - top) * 0.65);
        auto draw_rect = Painting::centeredGlyphRect(font, "!", glyph_rect);
        painter.drawText(draw_rect, Qt::AlignCenter, "!");

        painter.end();

        label_->setPixmap(icon);
    }
};

} // namespace Fernanda
