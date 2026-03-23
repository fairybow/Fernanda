/*
 * Fernanda is a plain text editor for fiction writing
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
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

#include "workspaces/WorkingDir.h"

/// TODO BA: Should this write (like NotepadRecovery)?
namespace Fernanda::NotebookLockfile {

namespace Internal {

    inline const auto FNX_KEY_ = QStringLiteral("fnx=");
    inline const auto DIR_KEY_ = QStringLiteral("working_dir=");
    inline const auto DIRTY_KEY_ = QStringLiteral("dirty_uuids=");

} // namespace Internal

struct Parsed
{
    Coco::Path fnxPath{};
    Coco::Path workingDirPath{};
    QSet<QString> dirtyUuids{};
};

inline Parsed fromData(const QByteArray& data)
{
    auto content = QString(data);
    Parsed parsed{};

    for (auto& line : content.split('\n', Qt::SkipEmptyParts)) {
        if (line.startsWith(Internal::FNX_KEY_))
            parsed.fnxPath = line.mid(Internal::FNX_KEY_.size());

        else if (line.startsWith(Internal::DIR_KEY_))
            parsed.workingDirPath = line.mid(Internal::DIR_KEY_.size());

        else if (line.startsWith(Internal::DIRTY_KEY_)) {
            for (auto& uuid : line.mid(Internal::DIRTY_KEY_.size())
                                  .split(',', Qt::SkipEmptyParts)) {
                parsed.dirtyUuids << uuid;
            }
        }
    }

    return parsed;
}

/// TODO BA: Should WorkingDir store UUIDs if adopted? Should it also store
/// original FNX path?
inline QByteArray toData(
    const Coco::Path& fnxPath,
    const WorkingDir& workingDir,
    const QStringList& dirtyUuids)
{
    QString content{};

    content += Internal::FNX_KEY_ + fnxPath.toQString() + "\n";
    content += Internal::DIR_KEY_ + workingDir.path().toQString() + "\n";
    content += Internal::DIRTY_KEY_ + dirtyUuids.join(',') + "\n";

    return content.toUtf8();
}

} // namespace Fernanda::NotebookLockfile
