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
// extensions
namespace Fernanda::MagicBytes {

enum Kind
{
    NoSignature = 0,
    Png,
    SevenZip,
    Rtf,
    Pdf,
    Gif,
    Jpg,
    Zip // covers .docx (Word)
};

inline Kind type(const Coco::Path& path)
{
    struct Signature
    {
        Kind kind;
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
        return NoSignature; // Maybe replace with Error value?
    }

    // Read enough bytes to cover the longest signature (8 bytes for PNG)
    auto file_header = file.read(8);

    if (file_header.isEmpty()) {
        INFO("Empty file: {}", path);
        return NoSignature; // Maybe replace with Error value?
    }

    // Return kind if known
    for (const auto& [kind, bytes, length] : signatures) {
        if (file_header.startsWith(QByteArrayView(bytes, length))) return kind;
    }

    return NoSignature;
}

inline bool is(Kind kind, const Coco::Path& path) { return kind == type(path); }

} // namespace Fernanda::MagicBytes
