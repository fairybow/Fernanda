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
#include <QTimer>

#include "Debug.h"

namespace Fernanda {

template <typename SlotT>
inline void timer(int msecs, QObject* parent, SlotT slot)
{
    QTimer::singleShot(msecs, parent, slot);
}

namespace Timers {

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

    private:
        QTimer* timer_ = new QTimer(this);
    };

    using Debouncer = Delayer;

} // namespace Timers

} // namespace Fernanda
