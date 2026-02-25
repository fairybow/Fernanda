/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QSaveFile>
#include <QString>

#include "Coco/Bool.h"
#include "Coco/Path.h"

#include "Debug.h"

namespace Fernanda::Io {

COCO_BOOL(CreateDirs);

inline QByteArray read(const Coco::Path& path)
{
    if (path.isEmpty()) {
        INFO("Path empty!");
        return {};
    }

    if (!path.exists()) {
        INFO("Path {} not found!", path);
        return {};
    }

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        auto err = file.errorString();
        WARN("Failed to open {} for reading (Error: {})!", path, err);
        return {};
    }

    return file.readAll();
}

inline bool write(
    const QByteArray& data,
    const Coco::Path& path,
    CreateDirs createDirs = CreateDirs::Yes)
{
    if (path.isEmpty()) {
        INFO("Path empty!");
        return false;
    }

    if (createDirs) {
        auto parent_path = path.parent();
        if (!parent_path.exists()) {
            if (!Coco::mkdir(parent_path)) {
                WARN("Failed to create directory at {}!", parent_path);
                return false;
            }
        }
    }

    QSaveFile file(path.toQString());

    if (!file.open(QIODevice::WriteOnly)) {
        auto err = file.errorString();
        WARN("Failed to open {} for writing (Error: {})!", path, err);
        return false;
    }

    auto written = file.write(data);

    if (written != data.size()) {
        WARN("Failed to write all data to file at {}!", path);
        return false;
    }

    if (!file.commit()) {
        auto err = file.errorString();
        WARN("Failed to commit file at {} (Error: {})!", path, err);
        return false;
    }

    return true;
}

} // namespace Fernanda::Io
