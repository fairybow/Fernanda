/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QMouseEvent>
#include <QObject>
#include <QTreeView>
#include <QWidget>

#include "core/Debug.h"

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
        // TODO: Would like to NOT unselect after a drag though (and currently
        // this handles both left and right clicks, so a drag can end in a right
        // click on empty space...)
        if (!indexAt(event->pos()).isValid()) {
            clearSelection();
            setCurrentIndex({});
        }

        QTreeView::mousePressEvent(event);
    }

private:
    void setup_()
    {
        setIndentation(20);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setEditTriggers(
            QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed); // F2 (standard)

        // Drag and drop
        setDragEnabled(true);
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDefaultDropAction(Qt::MoveAction);
    }
};

} // namespace Fernanda
