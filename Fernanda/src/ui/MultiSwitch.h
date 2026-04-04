#pragma once

#include <QPaintEvent>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

class MultiSwitch : public QWidget
{
    Q_OBJECT

public:
    MultiSwitch(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~MultiSwitch() override { TRACER; }

protected:
    /// TODO STYLE
    virtual void paintEvent(QPaintEvent* event) override
    {
        //
    }

private:
    void setup_()
    {
        //
    }
};

} // namespace Fernanda
