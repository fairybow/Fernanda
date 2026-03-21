/*
 * Fernanda is a plain text editor for fiction writing
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

#include <chrono>
#include <string>

#include <QObject>
#include <QTimer>

#include "core/Debug.h"

namespace Fernanda::Time {

struct LocalTime
{
    std::chrono::local_seconds seconds;
    int milliseconds;
};

inline LocalTime now()
{
    auto now = std::chrono::system_clock::now();
    auto zone = std::chrono::current_zone();
    auto local_time = zone->to_local(now);
    auto since_epoch = local_time.time_since_epoch();

    auto secs = std::chrono::floor<std::chrono::seconds>(local_time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  since_epoch % std::chrono::seconds{ 1 })
                  .count();

    return { secs, static_cast<int>(ms) };
}

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

template <typename SlotT>
inline QTimer* newQTimer(QObject* parent, SlotT slot, int msec = -1)
{
    auto timer = new QTimer(parent);
    if (msec >= 0) timer->setInterval(msec);
    QObject::connect(timer, &QTimer::timeout, parent, slot);
    return timer;
}

using Ticker = QTimer;
using Delayer = QTimer;
using Debouncer = QTimer;

// Non-single-shot (polling) member
template <typename SlotT>
inline Ticker* newTicker(QObject* parent, SlotT slot, int msec = -1)
{
    auto timer = newQTimer<SlotT>(parent, slot, msec);
    timer->setSingleShot(false);
    return timer;
}

// Single-shot delay member
template <typename SlotT>
inline Delayer* newDelayer(QObject* parent, SlotT slot, int msec = -1)
{
    auto timer = newQTimer<SlotT>(parent, slot, msec);
    timer->setSingleShot(true);
    return timer;
}

// Single-shot delay member
template <typename SlotT>
inline Debouncer* newDebouncer(QObject* parent, SlotT slot, int msec = -1)
{
    return newDelayer<SlotT>(parent, slot, msec);
}

} // namespace Fernanda::Time
