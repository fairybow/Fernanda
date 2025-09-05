/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

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
    // void fileDoubleClicked(const Coco::Path& path); // To open files
    // void folderClicked(const Coco::Path& path);
    // void folderDoubleClicked(const Coco::Path& path);

    /// Perhaps emit an "itemDoubleClicked" signal and send the model index to
    /// the module, since our TreeView will be using one of two models

private:
    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
