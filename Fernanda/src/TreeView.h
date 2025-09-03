#pragma once

#include <QTreeView>
#include <QWidget>

#include "Coco/Debug.h"

namespace Fernanda {

//...
class TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeView(QWidget* parent = nullptr)
        : QTreeView(parent)
    {
        initialize_();
    }

    virtual ~TreeView() override { COCO_TRACER; }

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
