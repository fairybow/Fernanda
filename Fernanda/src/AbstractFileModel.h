/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include "Coco/Path.h"

#include "FileMeta.h"

namespace Fernanda {

class AbstractFileModel : public QObject
{
    Q_OBJECT

public:
    explicit AbstractFileModel(
        const Coco::Path& path,
        QObject* parent = nullptr)
        : QObject(parent)
        , meta_(new FileMeta(path, this))
    {
    }

    virtual ~AbstractFileModel() = default;

    FileMeta* meta() const noexcept { return meta_; }
    virtual QString preferredExtension() const { return {}; }

    virtual QByteArray data() const = 0;
    virtual bool supportsModification() const = 0;

    virtual void setData(const QByteArray& data) {}
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
