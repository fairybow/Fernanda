#pragma once

#include <QObject>
#include <QTimer>

#include "Coco/Debug.h"

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
