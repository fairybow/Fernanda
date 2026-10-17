/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"

namespace Fernanda::Ui {

// A horizontal slider with its current value shown to its right: a fixed range
// and starting value at construction, and valueChanged.
//
// The readout is fixed to the width of the widest value in the range, so it
// doesn't jitter as the digits change, and right-aligned so the numbers line
// up. Tracking is on: valueChanged fires as the handle moves, so the setting
// applies live while dragging (the Vault debounces the save)
class DisplaySlider : public QWidget
{
    Q_OBJECT

public:
    DisplaySlider(int minimum, int maximum, int value, QWidget* parent)
        : QWidget(parent)
    {
        setup_(minimum, maximum, value);
    }

    ~DisplaySlider() override { TRACER; }

    [[nodiscard]] int value() const { return slider_->value(); }

signals:
    void valueChanged(int value);

private:
    QSlider* slider_ = new QSlider(Qt::Horizontal, this);
    QLabel* readout_ = new QLabel(this);

    void setup_(int minimum, int maximum, int value)
    {
        slider_->setRange(minimum, maximum);
        slider_->setValue(qBound(minimum, value, maximum));
        slider_->setMinimumWidth(DISPLAY_SLIDER_MIN_WIDTH);

        auto metrics = readout_->fontMetrics();
        auto widest = qMax(
            metrics.horizontalAdvance(QString::number(minimum)),
            metrics.horizontalAdvance(QString::number(maximum)));
        readout_->setFixedWidth(widest);
        readout_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        readout_->setNum(slider_->value());

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(DISPLAY_SLIDER_READOUT_SPACING);
        layout->addWidget(slider_, 1);
        layout->addWidget(readout_, 0);

        connect(slider_, &QSlider::valueChanged, this, [this](int value) {
            readout_->setNum(value);
            emit valueChanged(value);
        });
    }
};

} // namespace Fernanda::Ui
