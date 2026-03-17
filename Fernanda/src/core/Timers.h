/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <chrono>

#include <QObject>
#include <QTimer>

#include "core/Debug.h"

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
