/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QSlider>
#include <QString>
#include <QWidget>
#include <Qt>

#include "Debug.h"

namespace Fernanda {

// Wraps a QSlider with QLabel showing the current value
//
// TODO: Prevent display width change from moving the slider itself (probably
// just predict size based on max amount set and add it in as buffer)
class DisplaySlider : public QWidget
{
    Q_OBJECT

public:
    explicit DisplaySlider(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~DisplaySlider() override { TRACER; }

    int minimum() const { return slider_->minimum(); }
    int maximum() const { return slider_->maximum(); }
    int value() const { return slider_->value(); }
    int tickInterval() const { return slider_->tickInterval(); }

    void setMinimum(int min)
    {
        slider_->setMinimum(min);
        setDisplayText_();
    }

    void setMaximum(int max)
    {
        slider_->setMaximum(max);
        setDisplayText_();
    }

    void setRange(int min, int max)
    {
        slider_->setRange(min, max);
        setDisplayText_();
    }

    void setValue(int value)
    {
        slider_->setValue(value);
        setDisplayText_();
    }

    void setTickInterval(int tickInterval)
    {
        slider_->setTickInterval(tickInterval);
    }

signals:
    void valueChanged(int value);
    void rangeChanged(int min, int max);
    void sliderMoved(int value);
    void sliderPressed();
    void sliderReleased();

private:
    QSlider* slider_ = new QSlider(Qt::Horizontal, this);
    QLabel* display_ = new QLabel(this);

    void setup_()
    {
        // Setup
        slider_->setTickPosition(QSlider::NoTicks);
        slider_->setTickInterval(1);
        slider_->setRange(0, 100);

        // Populate
        slider_->setValue(100);
        setDisplayText_();

        // Layout
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0); // Keep spacing but no margin
        layout->addWidget(slider_);
        layout->addWidget(display_);

        // Connect
        connect(slider_, &QSlider::valueChanged, this, [&](int value) {
            setDisplayText_();
            emit valueChanged(value);
        });

        connect(slider_, &QSlider::rangeChanged, this, [&](int min, int max) {
            emit rangeChanged(min, max);
        });

        connect(slider_, &QSlider::sliderMoved, this, [&](int value) {
            emit sliderMoved(value);
        });

        connect(slider_, &QSlider::sliderPressed, this, [&] {
            emit sliderPressed();
        });

        connect(slider_, &QSlider::sliderReleased, this, [&] {
            emit sliderReleased();
        });
    }

    void setDisplayText_()
    {
        display_->setText(QString::number(slider_->value()));
    }
};

} // namespace Fernanda
