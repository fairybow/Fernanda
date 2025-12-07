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

#include "Coco/Path.h"

#include "Debug.h"

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

    virtual ~FileMeta() override { TRACER; }

    bool isOnDisk() const noexcept { return !path_.isEmpty(); }
    Coco::Path path() const noexcept { return path_; }
    QString title() const noexcept { return title_; }
    QString toolTip() const noexcept { return toolTip_; }

    void setPath(const Coco::Path& path)
    {
        if (path_ == path) return;
        Coco::Path old = path_;
        path_ = path;
        emit pathChanged(old, path_);
        updateDerivedProperties_();
    }

    void setTitleOverride(const QString& title)
    {
        if (titleOverride_ == title) return;
        titleOverride_ = title;
        updateDerivedProperties_();
    }

    void clearTitleOverride()
    {
        if (titleOverride_.isEmpty()) return;
        titleOverride_.clear();
        updateDerivedProperties_();
    }

signals:
    void changed();
    void pathChanged(const Coco::Path& old, const Coco::Path& now);

private:
    Coco::Path path_;

    QString title_{};
    QString titleOverride_{};
    QString toolTip_{};

    void updateDerivedProperties_()
    {
        QString old_title = title_;
        QString old_tooltip = toolTip_;

        // Update title: custom title > path stem > "Untitled"
        if (!titleOverride_.isEmpty())
            title_ = titleOverride_;
        else if (isOnDisk())
            title_ = path_.stemQString();
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
