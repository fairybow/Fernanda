/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QObject>

#include "Coco/Path.h"

#include "FileMeta.h"

namespace Fernanda {

// Interface for document content and editing operations, managing document
// state, modification tracking, and undo/redo functionality without handling
// metadata
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

    virtual QByteArray data() const { return {}; }
    virtual void setData(const QByteArray& data) {}
    virtual bool supportsModification() const { return false; }
    virtual bool isModified() const { return false; }
    virtual void setModified(bool modified) {}
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
