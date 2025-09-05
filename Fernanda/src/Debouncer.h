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

#include "Coco/Debug.h"

// Utility class for initializing a debouncing timer and connecting it to a slot
class Debouncer : public QTimer
{
    Q_OBJECT

public:
    template <typename SlotT>
    Debouncer(int interval, QObject* parent, SlotT slot)
        : QTimer(parent)
    {
        setSingleShot(true);
        setInterval(interval);
        connect(this, &QTimer::timeout, parent, slot);
    }

    virtual ~Debouncer() override { COCO_TRACER; }
};
