/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include <QByteArray>
#include <QObject>
#include <QString>

#include <Coco/Path.h>

#include "models/FileMeta.h"

namespace Fernanda {

class AbstractFileModel : public QObject
{
    Q_OBJECT

public:
    /// TODO NF: See note over TextFileModel ctor
    explicit AbstractFileModel(
        FileTypes::Kind fileType,
        const Coco::Path& path,
        QObject* parent = nullptr)
        : QObject(parent)
        , meta_(new FileMeta(fileType, path, this))
    {
    }

    virtual ~AbstractFileModel() = default;

    FileMeta* meta() const noexcept { return meta_; }

    // These two are the contract. The rest is optional:

    virtual QByteArray data() const = 0;
    virtual void setData(const QByteArray& data) = 0;

    virtual bool isUserEditable() const { return false; }
    virtual bool hasUndo() const { return false; }
    virtual bool hasRedo() const { return false; }
    virtual void undo() {}
    virtual void redo() {}

    virtual bool isModified() const { return isModified_; }

    virtual void setModified(bool modified)
    {
        if (isModified_ == modified) return;
        isModified_ = modified;
        emit modificationChanged(modified);
    }

signals:
    void modificationChanged(bool modified);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

private:
    FileMeta* meta_;
    bool isModified_ = false;
};

} // namespace Fernanda
