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

private:
    void setup_()
    {
        //
    }
};

} // namespace Fernanda
