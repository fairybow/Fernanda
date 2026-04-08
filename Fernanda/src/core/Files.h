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

#include <QList>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

#include "core/MagicBytes.h"
#include "core/Tr.h"

namespace Fernanda::Files {

using namespace Qt::StringLiterals;

enum Type
{
    // Begin openable
    PlainText = 0,
    Notebook, // (Not plain text; only openable by Application)

    // (Special plain text)
    Markdown,
    Fountain,
    Html, // (not yet openable as intended)

    Pdf,

    // Images
    Png,
    Jpeg,
    Gif,
    Tiff,
    Bmp,
    WebP,
    // End openable

    // Importable
    MicrosoftWord,
    RichText, // (not yet implemented)

    // No special handling (yet or ever)
    WindowTheme,
    EditorTheme,

    // Planned
    // Corkboard,
};

inline QList<Type> workspaceCreatableTypes()
{
    return { PlainText, Markdown, Fountain };
}

inline QList<Type> importableTypes() { return { MicrosoftWord, RichText }; }

namespace Internal {

    // First entry per Type is the canonical extension
    struct ExtensionEntry_
    {
        Type type;
        const char* extension;
    };

    constexpr ExtensionEntry_ EXTENSIONS_[] = {
        { PlainText, ".txt" },
        { Notebook, ".fnx" },
        { Markdown, ".md" },
        { Markdown, ".markdown" },
        { Fountain, ".fountain" },
        { Html, ".html" },
        { Html, ".htm" },
        { Pdf, ".pdf" },
        { Png, ".png" },
        { Jpeg, ".jpeg" },
        { Jpeg, ".jpg" },
        { Gif, ".gif" },
        { Tiff, ".tiff" },
        { Tiff, ".tif" },
        { Bmp, ".bmp" },
        { WebP, ".webp" },
        { MicrosoftWord, ".docx" },
        { RichText, ".rtf" },
        { WindowTheme, ".fernanda_window" },
        { EditorTheme, ".fernanda_editor" },
    };

} // namespace Internal

constexpr Type fromMagicBytes(MagicBytes::Type type)
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

// Resolve file type from path. Unrecognized extensions fall through to
// PlainText
inline Type fromPath(const Coco::Path& path)
{
    auto ext = path.extQString().toLower();

    for (const auto& [type, known_ext] : Internal::EXTENSIONS_) {
        if (ext == known_ext) return type;
    }

    return PlainText;
}

constexpr const char* canonicalExt(Type type)
{
    for (const auto& [t, ext] : Internal::EXTENSIONS_) {
        if (t == type) return ext;
    }

    return ".txt";
}

inline const char* canonicalExt(const Coco::Path& path)
{
    return canonicalExt(fromPath(path));
}

inline QString name(Type type)
{
    switch (type) {
    default:
    case PlainText:
        return Tr::filesPlainText();
    case Notebook:
        return Tr::filesNotebook();
    case Markdown:
        return Tr::filesMarkdown();
    case Fountain:
        return Tr::filesFountain();
    case Html:
        return Tr::filesHtml();
    case Pdf:
        return Tr::filesPdf();
    case Png:
        return Tr::filesPng();
    case Jpeg:
        return Tr::filesJpeg();
    case Gif:
        return Tr::filesGif();
    case Tiff:
        return Tr::filesTiff();
    case Bmp:
        return Tr::filesBmp();
    case WebP:
        return Tr::filesWebP();
    case MicrosoftWord:
        return Tr::filesMsWord();
    case RichText:
        return Tr::filesRichText();
    case WindowTheme:
        return Tr::filesWindowTheme();
    case EditorTheme:
        return Tr::filesEditorTheme();
    }
}

inline QString filter() { return Tr::filesFilterAll() + u" (*)"_s; }

inline QString filter(Type type)
{
    return name(type) + u" (*"_s + canonicalExt(type) + u")"_s;
}

// Compound filter from a list, e.g., "Microsoft Word (Office Open XML) document
// (*.docx);;Rich Text (*.rtf)"
/// TODO NF: Add multiple extensions where relevant
inline QString filter(const QList<Type>& types)
{
    QStringList parts{};

    for (auto t : types) {
        parts << filter(t);
    }

    return parts.join(u";;"_s);
}

/// TODO NF: Generalize, if possible:

inline bool isFnxFile(const Coco::Path& path)
{
    return path.ext() == canonicalExt(Notebook)
           && MagicBytes::is(MagicBytes::Zip, path);
}

inline bool isDocxFile(const Coco::Path& path)
{
    return path.ext() == canonicalExt(MicrosoftWord)
           && MagicBytes::is(MagicBytes::Zip, path);
}

} // namespace Fernanda::Files
