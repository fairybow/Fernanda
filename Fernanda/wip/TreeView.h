#pragma once

#include <QObject>
#include <QTreeView>

#include "Coco/Debug.h"

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    using QTreeView::QTreeView;
    virtual ~TreeView() override { COCO_TRACER; }
};
