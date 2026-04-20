/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QObject>
#include <QString>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Files.h"
#include "core/Tr.h"

namespace Hearth {

using namespace Qt::StringLiterals;

// File metadata and path management separate from content (file path, display
// title, tooltip, on-disk status, and path changes)
class FileMeta : public QObject
{
    Q_OBJECT

public:
    explicit FileMeta(
        Files::Type fileType,
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

    Files::Type fileType() const noexcept { return fileType_; }
    Coco::Path path() const noexcept { return path_; }
    QString title() const noexcept { return title_; }
    QString toolTip() const noexcept { return toolTip_; }

    void setPath(const Coco::Path& path)
    {
        if (path_ == path) return;
        Coco::Path old = path_;
        path_ = path;
        isStale_ = false;
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

    bool isStale() const noexcept { return isStale_; }

    void markStale()
    {
        if (isStale_) return;
        isStale_ = true;
        updateDerivedProperties_();
    }

    void clearStale()
    {
        if (!isStale_) return;
        isStale_ = false;
        updateDerivedProperties_();
    }

    QString preferredExt() const
    {
        return isOnDisk() ? path_.extQString() : Files::canonicalExt(fileType_);
    }

signals:
    void changed();
    void pathChanged(const Coco::Path& old, const Coco::Path& now);

private:
    Files::Type fileType_;
    Coco::Path path_;

    // File was on-disk (path_ not empty) but was moved, deleted, or renamed
    // externally
    bool isStale_ = false;

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
            title_ = u"Untitled"_s;
        }

        auto tool_tip_fmt = u"Title: %0\nPath: %1\nType: %2"_s;
        auto type_name = Files::name(fileType_);

        QString path_str{};

        if (isOnDisk()) {
            path_str = path_.prettyQString();
            if (isStale_) path_str += " " + Tr::fileMetaStale();
        } else {
            path_str = Tr::fileMetaNotOnDisk();
        }

        toolTip_ = tool_tip_fmt.arg(title_).arg(path_str).arg(type_name);

        // Emit single signal if anything changed
        if (title_ != old_title || toolTip_ != old_tooltip) emit changed();
    }
};

} // namespace Hearth
