/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QFileSystemModel>
#include <QMimeData>
#include <QModelIndex>

#include <Coco/Path.h>

#include "core/Debug.h"

namespace Fernanda {

class NotepadFileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    explicit NotepadFileSystemModel(QObject* parent = nullptr)
        : QFileSystemModel(parent)
    {
        setup_();
    }

    virtual ~NotepadFileSystemModel() override { TRACER; }

    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        auto f = QFileSystemModel::flags(index);
        if (isDir(index)) f &= ~Qt::ItemIsEditable;
        return f;
    }

    virtual bool dropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        int row,
        int column,
        const QModelIndex& parent) override
    {
        // Only intercept moves (copies don't invalidate the source path)
        if (action != Qt::MoveAction || !data || !parent.isValid())
            return QFileSystemModel::dropMimeData(
                data,
                action,
                row,
                column,
                parent);

        // Snapshot source paths before the base class moves them
        Coco::PathList old_paths{};
        for (auto& url : data->urls()) {
            auto local = url.toLocalFile();
            if (!local.isEmpty()) old_paths << local;
        }

        auto success =
            QFileSystemModel::dropMimeData(data, action, row, column, parent);

        if (success) {
            auto dest_dir = Coco::Path(filePath(parent));
            if (!dest_dir.isDir()) {
                FATAL(
                    "Drop target [{}] is not a directory after successful base "
                    "move",
                    dest_dir);
            }

            for (auto& old_path : old_paths) {
                auto new_path = dest_dir / old_path.name();
                if (old_path != new_path) emit fileMoved(old_path, new_path);
            }
        }

        return success;
    }

signals:
    void fileMoved(const Coco::Path& old, const Coco::Path& now);

private:
    void setup_()
    {
        //
    }
};

} // namespace Fernanda
