/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QString>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/FileTypes.h"
#include "core/Tr.h"

namespace Fernanda {

// File metadata and path management separate from content (file path, display
// title, tooltip, on-disk status, and path changes)
class FileMeta : public QObject
{
    Q_OBJECT

public:
    explicit FileMeta(
        FileTypes::Kind fileType,
        const Coco::Path& path = {},
        QObject* parent = nullptr)
        : QObject(parent)
        , fileType_(fileType)
        , path_(path)
    {
        updateDerivedProperties_();
    }

    virtual ~FileMeta() override { TRACER; }

    bool isOnDisk() const noexcept { return !path_.isEmpty(); }

    FileTypes::Kind fileType() const noexcept { return fileType_; }
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

    QString preferredExt() const
    {
        return !path_.isEmpty() ? path_.extQString()
                                : FileTypes::canonicalExt(fileType_);
    }

signals:
    void changed();
    void pathChanged(const Coco::Path& old, const Coco::Path& now);

private:
    FileTypes::Kind fileType_;
    Coco::Path path_;

    QString title_{};
    QString titleOverride_{};
    QString toolTip_{};

    void updateDerivedProperties_()
    {
        QString old_title = title_;
        QString old_tooltip = toolTip_;

        // Update title: custom title > path stem > "Untitled"
        if (!titleOverride_.isEmpty()) {
            title_ = titleOverride_;
        } else if (isOnDisk()) {
            title_ = path_.stemQString();
        } else {
            // TODO: Tr-aware in future?
            title_ = QStringLiteral("Untitled");
        }

        auto tool_tip_fmt = QStringLiteral("Title: %0\nPath: %1\nType: %2");
        auto type_name = FileTypes::name(fileType_);

        toolTip_ = tool_tip_fmt.arg(title_)
                       .arg(
                           isOnDisk() ? path_.prettyQString()
                                      : Tr::fileMetaNotOnDisk())
                       .arg(type_name);

        // Emit single signal if anything changed
        if (title_ != old_title || toolTip_ != old_tooltip) emit changed();
    }
};

} // namespace Fernanda
