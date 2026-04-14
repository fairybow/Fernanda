#pragma once

#include <QSplitter>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

class TabSurface : public QWidget
{
    Q_OBJECT

public:
    explicit TabSurface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TabSurface() override { TRACER; }

private:
    void setup_()
    {
        //
    }
};

} // namespace Fernanda
