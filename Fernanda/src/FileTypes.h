/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <utility>

#include <QByteArray>
#include <QByteArrayView>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>

#include "Coco/Log.h"
#include "Coco/Path.h"

// Detects file types by analyzing file signatures (magic bytes) rather than
// extensions, for potential support of various formats including text, images,
// archives, and documents across the Workspace. Currently, we use this to
// explicitly detect 7zip archives and also as a kind of filter in FileService
// (any type present in the enums here, like GIF or JPG, will not resolve to
// PlainText and open as NoOp), though this will likely change, and it will only
// be used to open special files/views (and not block anything else unless
// needed)
namespace Fernanda::FileTypes {

// Unknown types treated as plain text
enum HandledType
{
    PlainText = 0,
    Png,
    SevenZip,
    Rtf,
    Pdf,
    Gif,
    Jpg,
    Zip // covers .docx (Word)
};

inline HandledType type(const Coco::Path& path)
{
    // https://en.wikipedia.org/wiki/List_of_file_signatures
    static const QList<std::pair<HandledType, QByteArray>> data = {
        // Check longer signatures first to avoid false positives. Note:
        // .docx is a .zip file!
        { Png, QByteArray::fromHex("89504E470D0A1A0A") }, // 8 bytes
        { SevenZip, QByteArray::fromHex("377ABCAF271C") }, // 6
        { Rtf, QByteArray::fromHex("7B5C72746631") }, // 6
        { Pdf, QByteArray::fromHex("255044462D") }, // 5
        { Gif, QByteArray::fromHex("47494638") }, // 4
        { Jpg, QByteArray::fromHex("FFD8FF") }, // 3
        { Zip, QByteArray::fromHex("504B") }, // 2
    };

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        ///COCO_LOG(QString("Unable to open file: %0").arg(path.toQString()));
        return PlainText; // Maybe replace with Error value
    }

    // Read enough bytes to cover the longest signature (8 bytes for PNG)
    auto file_header = file.read(8);

    if (file_header.isEmpty()) {
        ///COCO_LOG(QString("Empty file: %0").arg(path.toQString()));
        return PlainText;
    }

    // Check each signature
    for (const auto& [type, signature] : data)
        if (file_header.startsWith(signature)) return type;

    // No signature matched, assume plain text
    return PlainText;
}

inline bool is(HandledType fileType, const Coco::Path& path)
{
    return fileType == type(path);
}

} // namespace Fernanda::FileTypes
