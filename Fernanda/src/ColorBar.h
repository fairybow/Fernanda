#pragma once

#include <QObject>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

namespace Fernanda {

// A colorful gradient progress bar for visual feedback on save and startup
class ColorBar : public QWidget
{
    Q_OBJECT

public:
    explicit ColorBar(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        initialize_();
    }

    virtual ~ColorBar() override { COCO_TRACER; }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
