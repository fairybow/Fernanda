#pragma once

#include <QTreeView>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Path.h"

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

signals:
    /// We may handle expanding/closing folders in the tree view itself?
    // void fileClicked(const Coco::Path& path);
    void fileDoubleClicked(const Coco::Path& path); // To open files
    // void folderClicked(const Coco::Path& path);
    // void folderDoubleClicked(const Coco::Path& path);

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
