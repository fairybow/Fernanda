/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include "core/Debug.h"
#include "core/Hash.h"
#include "core/Time.h"

namespace Fernanda::Backup {

namespace Internal {

    // YYYYMMDD-HHmmss-mmm (local time)
    inline QString timestamp_()
    {
        auto now = Time::now();
        auto days = std::chrono::floor<std::chrono::days>(now.seconds);
        std::chrono::year_month_day ymd{ days };
        std::chrono::hh_mm_ss hms{ now.seconds - days };

        return QString::asprintf(
            "%04d%02d%02d-%02d%02d%02d-%03d",
            static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()),
            static_cast<unsigned>(ymd.day()),
            static_cast<int>(hms.hours().count()),
            static_cast<int>(hms.minutes().count()),
            static_cast<int>(hms.seconds().count()),
            now.milliseconds);
    }

    // ("{hash}_{stem}.")
    inline QString prefix_(const Coco::Path& filePath)
    {
        return Hash::fromPath(filePath) + "_" + filePath.stemQString() + ".";
    }

    // ("{hash}_{stem}.{timestamp}{.ext}")
    inline QString fileName_(const QString& prefix, const Coco::Path& filePath)
    {
        return prefix + timestamp_() + filePath.extQString();
    }

    /// TODO BA: Rename?
    struct Data_
    {
        QString prefix{};
        QString fileName{};
    };

    inline Data_ data_(const Coco::Path& filePath)
    {
        auto prefix = prefix_(filePath);
        return { prefix, fileName_(prefix, filePath) };
    }

} // namespace Internal

// Copies filePath into backupDir before overwrite, then prunes the oldest
// backups sharing the same source prefix beyond pruneCap. Failure logs a
// warning but never throws (backup must not block saving)
inline void createAndPrune(
    const Coco::Path& filePath,
    const Coco::Path& backupDir,
    int pruneCap)
{
    if (!filePath.exists()) return;
    if (!backupDir.exists()) {
        WARN("Backup directory [{}] doesn't exist!", filePath, backupDir);
        return;
    }

    // Create
    auto data = Internal::data_(filePath);
    auto backup_path = backupDir / data.fileName;

    if (!Coco::copy(filePath, backup_path)) {
        WARN("Backup failed for {} (target: {})", filePath, backup_path);
        return;
    }

    INFO("Backup created: {}", backup_path);

    // Prune
    if (pruneCap < 1) return;

    auto all_files = Coco::filePaths(backupDir);
    QStringList matches{};

    for (auto& path : all_files) {
        auto file_name = path.nameQString();
        if (file_name.startsWith(data.prefix)) matches << file_name;
    }

    if (matches.size() <= pruneCap) return;

    matches.sort(); // By timestamp

    auto to_remove = matches.size() - pruneCap;

    for (qsizetype i = 0; i < to_remove; ++i) {
        auto old_path = backupDir / matches[i];
        if (!Coco::remove(old_path))
            WARN("Failed to prune backup: {}", old_path);
        else
            INFO("Pruned backup: {}", old_path);
    }
}

} // namespace Fernanda::Backup
