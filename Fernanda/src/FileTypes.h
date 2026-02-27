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
#include <QByteArrayView>
#include <QFile>
#include <QIODevice>
#include <QString>

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
    struct Signature
    {
        HandledType type;
        const char* bytes;
        qsizetype length;
    };

    // https://en.wikipedia.org/wiki/List_of_file_signatures
    // Check longer signatures first to avoid false positives
    static constexpr Signature signatures[] = {
        { Png, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8 },
        { SevenZip, "\x37\x7A\xBC\xAF\x27\x1C", 6 },
        { Rtf, "\x7B\x5C\x72\x74\x66\x31", 6 },
        { Pdf, "\x25\x50\x44\x46\x2D", 5 },
        { Gif, "\x47\x49\x46\x38", 4 },
        { Jpg, "\xFF\xD8\xFF", 3 },
        { Zip, "\x50\x4B", 2 },
    };

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        INFO("Unable to open file: {}", path);
        return PlainText; // Maybe replace with Error value
    }

    // Read enough bytes to cover the longest signature (8 bytes for PNG)
    auto file_header = file.read(8);

    if (file_header.isEmpty()) {
        INFO("Empty file: {}", path);
        return PlainText;
    }

    // Check each signature
    for (const auto& [type, bytes, length] : signatures) {
        if (file_header.startsWith(QByteArrayView(bytes, length))) return type;
    }

    // No signature matched, assume plain text
    return PlainText;
}

inline bool is(HandledType fileType, const Coco::Path& path)
{
    return fileType == type(path);
}

} // namespace Fernanda::FileTypes
