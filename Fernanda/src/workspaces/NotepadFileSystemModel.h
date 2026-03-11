/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFileSystemModel>
#include <QModelIndex>

#include "core/Debug.h"

namespace Fernanda {

/// TODO FT: Possbly temporary: exists solely to prevent renaming of directories
/// while still allowing file renames
class NotepadFileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    using QFileSystemModel::QFileSystemModel;
    virtual ~NotepadFileSystemModel() override { TRACER; }

    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        auto f = QFileSystemModel::flags(index);
        if (isDir(index)) f &= ~Qt::ItemIsEditable;
        return f;
    }
};

} // namespace Fernanda
