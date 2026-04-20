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

#include <QString>
#include <QStringList>

#include <Coco/Path.h>

namespace Hearth::Disk {

inline void
prune(const Coco::Path& dir, const QString& prefix, const QString& ext, int cap)
{
    if (cap < 1) return;

    auto all_files = Coco::filePaths(dir);
    QStringList matches{};

    for (auto& path : all_files) {
        auto name = path.nameQString();
        if (name.startsWith(prefix) && name.endsWith(ext)) matches << name;
    }

    if (matches.size() <= cap) return;

    matches.sort();

    auto to_remove = matches.size() - cap;

    for (qsizetype i = 0; i < to_remove; ++i)
        Coco::remove(dir / matches[i]);
}

} // namespace Hearth::Disk
