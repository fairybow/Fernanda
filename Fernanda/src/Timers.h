/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <chrono>

#include <QObject>
#include <QTimer>

#include "Debug.h"

namespace Fernanda::Timers {

template <typename SlotT>
inline void delay(int msecs, const QObject* context, SlotT slot)
{
    QTimer::singleShot(msecs, context, slot);
}

template <typename SlotT> inline void delay(int msecs, SlotT slot)
{
    QTimer::singleShot(msecs, slot);
}

template <typename SlotT>
inline void onNextTick(const QObject* context, SlotT slot)
{
    QTimer::singleShot(0, context, slot);
}

template <typename SlotT> inline void onNextTick(SlotT slot)
{
    QTimer::singleShot(0, slot);
}

// Utility class for initializing a debouncing/delay timer and connecting it
// to a slot
class Delayer : public QObject
{
    Q_OBJECT

public:
    template <typename SlotT>
    Delayer(int interval, QObject* parent, SlotT slot)
        : QObject(parent)
    {
        timer_->setSingleShot(true);
        timer_->setInterval(interval);
        connect(timer_, &QTimer::timeout, parent, slot);
    }

    virtual ~Delayer() override { TRACER; }

    void start() { timer_->start(); }
    void stop() { timer_->stop(); }
    void setInterval(int msec) { timer_->setInterval(msec); }
    void setInterval(std::chrono::milliseconds value)
    {
        timer_->setInterval(value);
    }

private:
    QTimer* timer_ = new QTimer(this);
};

using Debouncer = Delayer;

} // namespace Fernanda::Timers
