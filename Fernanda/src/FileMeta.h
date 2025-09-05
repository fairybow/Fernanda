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
#include <QString>

#include "Coco/Debug.h"
#include "Coco/Path.h"

namespace Fernanda {

// File metadata and path management separate from content (file path, display
// title, tooltip, on-disk status, and path changes)
class FileMeta : public QObject
{
    Q_OBJECT

public:
    explicit FileMeta(const Coco::Path& path = {}, QObject* parent = nullptr)
        : QObject(parent)
        , path_(path)
    {
        updateDerivedProperties_();
    }

    virtual ~FileMeta() override { COCO_TRACER; }

    bool isOnDisk() const noexcept { return !path_.isEmpty(); }
    Coco::Path path() const noexcept { return path_; }
    QString title() const noexcept { return title_; }
    QString toolTip() const noexcept { return toolTip_; }

    void setPath(const Coco::Path& path)
    {
        if (path_ == path) return;
        path_ = path;
        updateDerivedProperties_();
        // emit pathChanged(path_);
    }

    void setTemporaryTitle(const QString& title)
    {
        if (temporaryTitle_ == title) return;
        temporaryTitle_ = title;
        updateDerivedProperties_();
    }

    void clearTemporaryTitle()
    {
        if (temporaryTitle_.isEmpty()) return;
        temporaryTitle_.clear();
        updateDerivedProperties_();
    }

signals:
    void changed();

private:
    Coco::Path path_;

    QString title_{};
    QString temporaryTitle_{};
    QString toolTip_{};

    void updateDerivedProperties_()
    {
        QString old_title = title_;
        QString old_tooltip = toolTip_;

        // Update title: path stem > custom title > "Untitled"
        if (isOnDisk())
            title_ = path_.stemQString();
        else if (!temporaryTitle_.isEmpty())
            title_ = temporaryTitle_;
        else
            title_ = "Untitled";

        // Update tooltip: full path if available, otherwise title + status
        if (isOnDisk())
            toolTip_ = path_.toQString();
        else
            toolTip_ = title_ + " [Not on disk]";

        // Emit single signal if anything changed
        if (title_ != old_title || toolTip_ != old_tooltip) emit changed();
    }
};

} // namespace Fernanda
