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
#include <QLabel>
#include <QPainter>
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
    explicit TabWidgetAlertWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TabWidgetAlertWidget() override { TRACER; }

    bool active() const noexcept { return active_; }

    void setAlert(const QString& message)
    {
        active_ = true;
        setFixedSize(ICON_SIZE_);
        setToolTip(message);
        setVisible(true);
    }

    void clearAlert()
    {
        active_ = false;
        setToolTip({});
        setVisible(false);
        setFixedSize(0, 0);
    }

private:
    static constexpr auto ICON_SIZE_ = QSize(16, 16);
    QLabel* label_ = new QLabel(this);
    bool active_ = false;

    void setup_()
    {
        setVisible(false);
        setFixedSize(0, 0);
        buildIcon_();
    }

    // TODO: StyleContext support
    // TODO: What else can move to Painting (shared with ControlField info
    // icon)?
    void buildIcon_()
    {
        auto total = qMin(ICON_SIZE_.width(), ICON_SIZE_.height());
        auto padding = total / 10.0;
        auto circle_size = total - padding * 2.0;
        auto color = Qt::darkRed;

        QPixmap icon(total, total);
        icon.fill(Qt::transparent);

        QPainter painter(&icon);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(QPen(color, total / 12.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(padding, padding, circle_size, circle_size));

        QFont font = painter.font();
        font.setPixelSize(qRound(total * 0.77));
        font.setBold(true);
        font.setFamily("Arial");
        font.setStyleHint(QFont::SansSerif);
        painter.setFont(font);

        painter.setPen(color);

        // TODO: "!" is a little too far up but unsure how to fix
        auto draw_rect = Painting::centeredGlyphRect(
            font,
            "!",
            QRectF(padding, padding, circle_size, circle_size));
        painter.drawText(draw_rect, Qt::AlignCenter, "!");

        painter.end();

        label_->setPixmap(icon);
    }
};

} // namespace Fernanda
