/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QMouseEvent>
#include <QObject>
#include <QTreeView>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget* parent = nullptr)
        : QTreeView(parent)
    {
        setup_();
    }

    virtual ~TreeView() override { TRACER; }

protected:
    virtual void mousePressEvent(QMouseEvent* event) override
    {
        // Allow clicking on invalid model indexes to unselect the currently
        // selected model index
        // TODO: Could shrink item widths to allow some space on the side to
        // unselect current
        if (!indexAt(event->pos()).isValid()) {
            clearSelection();
            setCurrentIndex({});
        }

        QTreeView::mousePressEvent(event);
    }

private:
    void setup_()
    {
        //...
    }
};

} // namespace Fernanda
