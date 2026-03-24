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
#include <QString>

#include <Coco/Path.h>

#include "core/Hash.h"
#include "core/Io.h"

namespace Fernanda::NotepadRecovery {

struct Entry
{
    Coco::Path originalPath{};
    QString title{};
    QByteArray buffer{};

    bool isOffDisk() const noexcept { return originalPath.isEmpty(); }
};

namespace Internal {

    inline const auto PATH_KEY_ = QStringLiteral("path=");
    inline const auto TITLE_KEY_ = QStringLiteral("title=");

    inline const auto BUFFER_NAME_ = QStringLiteral("buffer");
    inline const auto META_NAME_ = QStringLiteral("meta");
    inline const auto OFF_DISK_PREFIX_ = QStringLiteral("off-disk~");

    inline Entry read_(const Coco::Path& entryDir)
    {
        Entry entry{};
        entry.buffer = Io::read(entryDir / BUFFER_NAME_);

        auto meta =
            QString::fromUtf8(Io::read(entryDir / META_NAME_));

        for (auto& line : meta.split('\n', Qt::SkipEmptyParts)) {
            if (line.startsWith(PATH_KEY_)) {
                entry.originalPath = line.mid(PATH_KEY_.size());

            } else if (line.startsWith(TITLE_KEY_)) {
                entry.title = line.mid(TITLE_KEY_.size());
            }
        }

        return entry;
    }

} // namespace Internal

inline Coco::Path
entryDir(const Coco::Path& recoveryRoot, const Coco::Path& originalPath)
{
    // On-disk: hash of original path
    return recoveryRoot / Hash::fromPath(originalPath);
}

inline Coco::Path offDiskEntryDir(const Coco::Path& recoveryRoot)
{
    return recoveryRoot / (Internal::OFF_DISK_PREFIX_ + Random::token(8));
}

inline void write(
    const Coco::Path& entryDir,
    const Coco::Path& originalPath,
    const QString& title,
    const QByteArray& buffer)
{
    Coco::mkpath(entryDir);

    // Buffer
    Io::write(buffer, entryDir / Internal::BUFFER_NAME_);

    // Metadata
    QString meta{};
    meta += Internal::PATH_KEY_ + originalPath.toQString() + "\n";
    meta += Internal::TITLE_KEY_ + title + "\n";
    Io::write(meta.toUtf8(), entryDir / Internal::META_NAME_);
}

inline QList<Entry> readAll(const Coco::Path& recoveryRoot)
{
    QList<Entry> entries{};

    if (!recoveryRoot.exists()) return entries;

    for (auto& dir : Coco::paths(recoveryRoot))
        entries << Internal::read_(dir);

    return entries;
}

} // namespace Fernanda::NotepadRecovery
