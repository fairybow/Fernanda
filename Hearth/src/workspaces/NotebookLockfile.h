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

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

#include "core/Io.h"

/// TODO BA
namespace Fernanda::NotebookLockfile {

using namespace Qt::StringLiterals;

struct Entry
{
    Coco::Path fnxPath{};
    Coco::Path workingDirPath{};
    QSet<QString> dirtyUuids{};
};

inline const auto EXT = u".lock"_s;

namespace Internal {

    inline const auto FNX_KEY_ = u"fnx="_s;
    inline const auto DIR_KEY_ = u"working_dir="_s;
    inline const auto DIRTY_KEY_ = u"dirty_uuids="_s;

    inline QByteArray toData_(
        const Coco::Path& fnxPath,
        const Coco::Path& workingDirPath,
        const QSet<QString>& dirtyUuids)
    {
        QString content{};
        content += FNX_KEY_ + fnxPath.toQString() + u"\n"_s;
        content += DIR_KEY_ + workingDirPath.toQString() + u"\n"_s;
        content += DIRTY_KEY_
                   + QStringList(dirtyUuids.begin(), dirtyUuids.end()).join(',')
                   + u"\n"_s;
        return content.toUtf8();
    }

    inline Entry read_(const Coco::Path& lockfilePath)
    {
        auto data = Io::read(lockfilePath);
        auto content = QString::fromUtf8(data);
        Entry entry{};

        for (auto& line : content.split('\n', Qt::SkipEmptyParts)) {
            if (line.startsWith(FNX_KEY_)) {
                entry.fnxPath = line.mid(FNX_KEY_.size());
            } else if (line.startsWith(DIR_KEY_)) {
                entry.workingDirPath = line.mid(DIR_KEY_.size());
            } else if (line.startsWith(DIRTY_KEY_)) {
                for (auto& uuid :
                     line.mid(DIRTY_KEY_.size()).split(',', Qt::SkipEmptyParts))
                    entry.dirtyUuids << uuid;
            }
        }

        return entry;
    }

} // namespace Internal

inline Coco::Path
path(const Coco::Path& recoveryDir, const Coco::Path& workingDirPath)
{
    return recoveryDir / (workingDirPath.nameQString() + EXT);
}

/// TODO BA: Should WorkingDir store UUIDs if adopted? Should it also store
/// original FNX path?
inline void write(
    const Coco::Path& lockfilePath,
    const Coco::Path& fnxPath,
    const Coco::Path& workingDirPath,
    const QSet<QString>& dirtyUuids)
{
    Io::write(
        Internal::toData_(fnxPath, workingDirPath, dirtyUuids),
        lockfilePath);
}

inline Entry read(const Coco::Path& lockfilePath)
{
    return Internal::read_(lockfilePath);
}

// TODO: Could return bool and log. Would want to distinguish between lockfile
// not found or deletion failed, though?
inline void remove(const Coco::Path& lockfilePath)
{
    Coco::remove(lockfilePath);
}

inline QList<Entry> readAll(const Coco::Path& recoveryDir)
{
    QList<Entry> entries{};
    if (!recoveryDir.exists()) return entries;

    for (auto& path : Coco::filePaths({ recoveryDir }, { u"*"_s + EXT })) {
        entries << Internal::read_(path);
    }

    return entries;
}

} // namespace Fernanda::NotebookLockfile
