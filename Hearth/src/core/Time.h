/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QDateTime>
#include <QObject>
#include <QTimer>

namespace Hearth::Time {

struct LocalTime
{
    std::chrono::local_seconds seconds;
    int milliseconds;
};

inline LocalTime now()
{
    auto dt = QDateTime::currentDateTime();
    auto epoch = dt.toSecsSinceEpoch();
    auto ms = dt.time().msec();

    auto secs = std::chrono::local_seconds{ std::chrono::seconds{ epoch } };
    return { secs, ms };
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

} // namespace Hearth::Time
