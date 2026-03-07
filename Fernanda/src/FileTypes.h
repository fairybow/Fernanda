/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include "Coco/Path.h"

#include "MagicBytes.h"

// These are just constants relating the file types and extensions Fernanda
// "knows" about. Special handling is determined by FileService
// TODO: Most of this is planned / currently unsupported. This exists for the
// future
namespace Fernanda::FileTypes {

/// TODO FT: Can move out into Fernanda and rename FileType?
enum Kind
{
    PlainText = 0, // fallback + .txt (TODO: we can maybe not even check for
                   // .txt? just hold it as a canonical for Save As if needed,
                   // except we might not do that here...)

    // Special plaintext:

    Markdown,
    Fountain,
    FernandaWindowTheme,
    FernandaEditorTheme,
    FernandaCorkboard, // (Will probably be plaintext, like JSON or XML)

    // Magic bytes:

    Pdf,
    Png,
    Jpeg,
    Gif,
    Tiff,
    Bmp,
    WebP,

    // Special case:

    // FernandaNotebook (.fnx) handled by Fnx + Application
};

// First entry per Kind is the canonical extension
struct ExtensionEntry
{
    Kind kind;
    const char* ext;
};

constexpr ExtensionEntry extensions[] = {
    // Plain text:
    { PlainText, ".txt" },
    { Markdown, ".md" },
    { Fountain, ".fountain" },
    { FernandaCorkboard, ".fcb" },
    { FernandaWindowTheme, ".fernanda_window" },
    { FernandaEditorTheme,
      ".fernanda_editor" }, /// TODO FT: For the themes, we'll want to pull this
    /// extension from here (instead of hardcoding in
    /// Themes.h), but they AREN'T special handling
    /// right now - just text file (json)

    // Magic bytes:
    { Pdf, ".pdf" },
    { Png, ".png" },
    { Jpeg, ".jpeg" },
    { Jpeg, ".jpg" },
    { Gif, ".gif" },
    { Tiff, ".tiff" },
    { Bmp, ".bmp" },
    { WebP, ".webp" }
};

// Resolve file type from path. Unrecognized extensions fall through to
// PlainText
inline Kind fromPath(const Coco::Path& path)
{
    auto ext = path.extQString().toLower();
    for (const auto& [kind, knownExt] : extensions)
        if (ext == knownExt) return kind;

    return PlainText;
}

constexpr const char* canonicalExt(Kind kind)
{
    for (const auto& [k, ext] : extensions)
        if (k == kind) return ext;

    return ".txt";
}

inline const char* canonicalExt(const Coco::Path& path)
{
    return canonicalExt(fromPath(path));
}

constexpr Kind fromMagicBytes(MagicBytes::Type type)
{
    switch (type) {
    case MagicBytes::Pdf:
        return Pdf;
    case MagicBytes::Png:
        return Png;
    case MagicBytes::Jpeg:
        return Jpeg;
    case MagicBytes::Gif:
        return Gif;
    case MagicBytes::Tiff:
        return Tiff;
    case MagicBytes::Bmp:
        return Bmp;
    case MagicBytes::WebP:
        return WebP;
    default:
        return PlainText;
    }
}

} // namespace Fernanda::FileTypes
