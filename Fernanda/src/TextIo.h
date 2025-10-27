/*
 * Fernanda  Copyright (C) 2025  fairybow
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
#include <QStringConverter>
#include <QTextStream>

#include "Coco/Bool.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Debug.h"

namespace Fernanda::TextIo {

COCO_BOOL(CreateDirs);

inline QString read(
    const Coco::Path& path,
    QStringConverter::Encoding encoding = QStringConverter::Utf8)
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

    // QIODevice::Text automatically normalizes line endings. If we later
    // want to preserve original line endings, we'll need to remove that
    // flag and handle conversion manually
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto err = file.errorString();
        WARN("Failed to open {} for reading (Error: {})!", path, err);
        return {};
    }

    QTextStream in(&file); // QStringDecoder for BOM handling?
    in.setEncoding(encoding);
    return in.readAll();
}

inline bool write(
    const QString& text,
    const Coco::Path& path,
    CreateDirs createDirs = CreateDirs::Yes,
    QStringConverter::Encoding encoding = QStringConverter::Utf8)
{
    if (path.isEmpty()) {
        INFO("Path empty!");
        return {};
    }

    if (createDirs) {
        auto parent_path = path.parent();
        if (!parent_path.exists()) {
            if (!Coco::PathUtil::mkdir(parent_path)) {
                WARN("Failed to create directory at {}!", parent_path);
                return false;
            }
        }
    }

    QSaveFile file(path.toQString());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        auto err = file.errorString();
        WARN("Failed to open {} for writing (Error: {})!", path, err);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(encoding);
    out << text;

    if (out.status() != QTextStream::Ok) {
        WARN("Failed to write text to file at {}!", path);
        return false;
    }

    if (!file.commit()) {
        auto err = file.errorString();
        WARN("Failed to commit file at {} (Error: {})!", path, err);
        return false;
    }

    return true;
}

} // namespace Fernanda::TextIo
