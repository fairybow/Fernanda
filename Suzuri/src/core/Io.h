/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <optional>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QSaveFile>

#include <Coco/Bool.h>
#include <Coco/Debug.h>
#include <Coco/Path.h>

// Stateless file IO, as free functions: JSON and text IO are functions, not an
// object. Kept in Suzuri rather than Coco until proven — the Coco::Disk relic
// is deliberately not used here
namespace Fernanda::Io {

inline QByteArray read(const Coco::Path& path)
{
    if (path.isEmpty()) {
        DEBUG("Path empty!");
        return {};
    }

    if (!path.exists()) {
        DEBUG("Path {} not found!", path.prettyQString());
        return {};
    }

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        WARN(
            "Failed to open {} for reading (Error: {})!",
            path,
            file.errorString());
        return {};
    }

    return file.readAll();
}

// Like read(), but distinguishes an empty file from a read FAILURE. The
// reconcile path must never treat a transient failure — a file locked mid-write
// by an external tool — as "the file is now empty" and clobber a buffer.
// nullopt == couldn't read (skip/defer); an empty QByteArray == the file is
// genuinely empty
inline std::optional<QByteArray> tryRead(const Coco::Path& path)
{
    if (path.isEmpty()) {
        DEBUG("Path empty!");
        return std::nullopt;
    }

    if (!path.exists()) {
        DEBUG("Path {} not found!", path.prettyQString());
        return std::nullopt;
    }

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        WARN(
            "Failed to open {} for reading (Error: {})!",
            path,
            file.errorString());
        return std::nullopt;
    }

    return file.readAll();
}

COCO_BOOL(CreateDirs)

// QSaveFile writes to a temp file and renames on commit, so a crash mid-write
// never truncates the original. NB: that rename drops the path from
// QFileSystemWatcher, so the Vault re-adds the path after its own write and
// recognizes the resulting change event as its own (Vault::writeModel_)
//
// createDirs has no default: whether a write may invent missing folders is a
// real choice at each call site. Config files say Yes (.suzuri/ or appData()
// may not exist yet); a buffer's write and a new file say No, so a folder
// deleted or moved outside Suzuri is never recreated behind the user
inline bool
write(const QByteArray& data, const Coco::Path& path, CreateDirs createDirs)
{
    if (path.isEmpty()) {
        DEBUG("Path empty!");
        return false;
    }

    if (createDirs) {
        auto parent_path = path.parent();

        if (!parent_path.exists()) {
            if (!Coco::mkpath(parent_path)) {
                WARN("Failed to create directory at {}!", parent_path);
                return false;
            }
        }
    }

    QSaveFile file(path.toQString());

    if (!file.open(QIODevice::WriteOnly)) {
        auto err = file.errorString();
        WARN(
            "Failed to open {} for writing (Error: {})!",
            path.prettyQString(),
            err);
        return false;
    }

    auto written = file.write(data);

    if (written != data.size()) {
        WARN("Failed to write all data to file at {}!", path.prettyQString());
        return false;
    }

    if (!file.commit()) {
        auto err = file.errorString();
        WARN(
            "Failed to commit file at {} (Error: {})!",
            path.prettyQString(),
            err);
        return false;
    }

    return true;
}

} // namespace Fernanda::Io
