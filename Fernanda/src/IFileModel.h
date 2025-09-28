/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>

#include "Coco/Path.h"

#include "Enums.h"
#include "FileMeta.h"

namespace Fernanda {

// Abstract interface for document content and editing operations, managing
// document state, modification tracking, undo/redo functionality, and save
// operations without handling metadata
class IFileModel : public QObject
{
    Q_OBJECT

public:
    explicit IFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : QObject(parent)
        , meta_(new FileMeta(path, this))
    {
    }

    virtual ~IFileModel() = default;

    FileMeta* meta() const noexcept { return meta_; }

    virtual bool supportsModification() const { return false; }
    virtual SaveResult save() { return SaveResult::NoOp; }

    virtual SaveResult saveAs(const Coco::Path& newPath)
    {
        return SaveResult::NoOp;
    }

    virtual bool isModified() const { return false; }
    virtual bool hasUndo() const { return false; }
    virtual bool hasRedo() const { return false; }
    virtual void undo() {}
    virtual void redo() {}

signals:
    void modificationChanged(bool modified);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

private:
    FileMeta* meta_;
};

} // namespace Fernanda
