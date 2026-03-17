/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
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
#include <QByteArrayView>
#include <QFile>
#include <QIODevice>
#include <QString>

#include <Coco/Path.h>

#include "core/Debug.h"

// Detects file types by analyzing file signatures (magic bytes) rather than
// extensions
namespace Fernanda::MagicBytes {

enum Type
{
    NoKnownSignature = 0,
    Png,
    // SevenZip,
    Rtf,
    Pdf,
    Tiff,
    Gif,
    Jpeg,
    Bmp,
    Zip, // covers .docx (Word)

    // Compound:

    WebP
};

inline Type type(const Coco::Path& path)
{
    struct Signature
    {
        Type type;
        const char* bytes;
        qsizetype length;
    };

    // https://en.wikipedia.org/wiki/List_of_file_signatures
    // Check longer signatures first to avoid false positives
    static constexpr Signature signatures[] = {
        { Png, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8 },
        // { SevenZip, "\x37\x7A\xBC\xAF\x27\x1C", 6 },
        { Rtf, "\x7B\x5C\x72\x74\x66\x31", 6 },
        { Pdf, "\x25\x50\x44\x46\x2D", 5 },
        { Tiff, "\x49\x49\x2A\x00", 4 },
        { Tiff, "\x4D\x4D\x00\x2A", 4 },
        { Gif, "\x47\x49\x46\x38", 4 },
        { Jpeg, "\xFF\xD8\xFF", 3 },
        { Bmp, "\x42\x4D", 2 },
        { Zip, "\x50\x4B", 2 },
    };

    QFile file(path.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        INFO("Unable to open file: {}", path);
        return NoKnownSignature; // Maybe replace with Error value?
    }

    // Read enough bytes to cover the longest signature
    auto file_header = file.read(12);

    if (file_header.isEmpty()) {
        INFO("Empty file: {}", path);
        return NoKnownSignature; // Maybe replace with Error value?
    }

    // WEBP: RIFF container with WEBP marker at offset 8
    if (file_header.size() >= 12
        && file_header.startsWith(QByteArrayView("\x52\x49\x46\x46", 4))
        && QByteArrayView(file_header).sliced(8, 4)
               == QByteArrayView("\x57\x45\x42\x50", 4))
        return WebP;

    // Return type if known
    for (const auto& [type, bytes, length] : signatures) {
        if (file_header.startsWith(QByteArrayView(bytes, length))) return type;
    }

    return NoKnownSignature;
}

inline bool is(Type t, const Coco::Path& path) { return t == type(path); }

} // namespace Fernanda::MagicBytes
