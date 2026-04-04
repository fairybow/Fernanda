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

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

class NotebookColorChip : public QWidget
{
    Q_OBJECT

public:
    NotebookColorChip(const QString& name, QWidget* parent = nullptr)
        : QWidget(parent)
        , name_(name)
        , color_(colorFromName_(name))
    {
        setup_();
    }

    virtual ~NotebookColorChip() override { TRACER; }

protected:
    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        auto chip_rect = rect().adjusted(1, 1, -1, -1);
        auto radius = chip_rect.height() / 2.0;

        // Pill background
        QPainterPath path{};
        path.addRoundedRect(QRectF(chip_rect), radius, radius);
        painter.fillPath(path, color_);

        // Text
        painter.setPen(textColor_());
        painter.drawText(chip_rect, Qt::AlignCenter, name_);
    }

private:
    QString name_{};
    QColor color_{};

    inline static constexpr auto PADDING_H = 12;
    inline static constexpr auto PADDING_V = 2;

    static QColor colorFromName_(const QString& name)
    {
        auto hash = qHash(name);

        // Mix bits for better distribution (xorshift-style)
        hash ^= (hash << 13);
        hash ^= (hash >> 7);
        hash ^= (hash << 17);

        auto hue = static_cast<int>(hash % 360);
        auto sat = 140 + static_cast<int>((hash >> 8) % 60); // 140-199
        auto val = 130 + static_cast<int>((hash >> 16) % 70); // 130-199

        return QColor::fromHsv(hue, sat, val);
    }

    QColor textColor_() const { return color_.lighter(180); }

    void setup_()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QFontMetrics metrics(font());
        auto text_width = metrics.horizontalAdvance(name_);
        auto text_height = metrics.height();

        setFixedSize(
            text_width + (PADDING_H * 2),
            text_height + (PADDING_V * 2));
    }
};

} // namespace Fernanda
