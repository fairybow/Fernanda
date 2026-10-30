/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QLatin1StringView>
#include <QString>

#include <Coco/Path.h>

// The one authority on which files Fernanda opens, and as what.
// Extension-based, like Obsidian's own dispatch: no IO decides a type. Free
// functions in core so both sides of the GUI line can ask the same question —
// Vault::makeModel_ dispatches on typeOf, and VaultTreeModel leaves out
// whatever typeOf calls Unsupported. isHiddenName is the other half of what the
// listings show. One table, so what the tree shows and what the Vault will open
// can't drift apart.
//
// "Text" means LISTED as text — not "anything that isn't a PDF". An unlisted
// extension (a .docx, an .svg, a file with no extension at all) is
// Unsupported and never reaches a TextFileModel, where one stray keystroke
// would autosave mangled UTF-8 over the original
namespace Fernanda::FileTypes {

enum class Type
{
    Unsupported = 0,
    Text,
    Pdf,
    Image,
};

namespace Internal {

struct ExtensionEntry_
{
    Type type{};
    const char* extension = nullptr;
};

// Matched case-insensitively, so .TXT and .Md are the same types.
//
// Against Obsidian's image list (avif, bmp, gif, jpeg, jpg, png, svg, webp),
// TIFF is a Fernanda addition, and SVG and AVIF are deferred: SVG is vector and
// wants re-rendering per zoom level rather than a scaled bitmap; AVIF has no Qt
// imageformats plugin. The extension only DISPATCHES — ImageFileModel decodes
// by content, so a mislabeled file still decodes. TIFF and WebP decode through
// runtime plugins (qtiff / qwebp, Qt Image Formats); the table stays static
// regardless, and a missing plugin surfaces as the view's load-failure
// placeholder, not as a file the trees hide
inline constexpr ExtensionEntry_ EXTENSIONS_[] = {
    { Type::Text,  ".txt"      },
    { Type::Text,  ".md"       },
    { Type::Text,  ".markdown" },
    { Type::Text,  ".fountain" },
    { Type::Pdf,   ".pdf"      },
    { Type::Image, ".png"      },
    { Type::Image, ".jpg"      },
    { Type::Image, ".jpeg"     },
    { Type::Image, ".gif"      },
    { Type::Image, ".bmp"      },
    { Type::Image, ".tif"      },
    { Type::Image, ".tiff"     },
    { Type::Image, ".webp"     },
};

} // namespace Internal

// A path's type by its extension (std::filesystem semantics, via Coco::Path).
// Note that a name with a period but no real extension — "Chapter 1. The
// Start" — has the extension ". The Start" and is therefore Unsupported
[[nodiscard]] inline Type typeOf(const Coco::Path& path)
{
    auto ext = path.extQString();

    for (const auto& entry : Internal::EXTENSIONS_) {
        if (ext.compare(
                QLatin1StringView(entry.extension),
                Qt::CaseInsensitive) == 0) {
            return entry.type;
        }
    }

    return Type::Unsupported;
}

[[nodiscard]] inline bool isSupported(const Coco::Path& path)
{
    return typeOf(path) != Type::Unsupported;
}

// Is an entry hidden from Fernanda's listings — the vault trees and the
// FileSwitcher? Any name beginning with a dot: the vault marker .fernanda/,
// plus .git/, .gitignore, and every other dotfile or dot-folder, as Obsidian
// hides its own .obsidian/. A hidden folder hides everything beneath it.
// Name-based because on Windows a leading dot carries no hidden attribute, so
// QDir::Hidden never excludes these there.
//
// Not a type question, but it sits beside isSupported because the same two
// listings ask both, and one header keeps them from drifting apart
[[nodiscard]] inline bool isHiddenName(const QString& name)
{
    return name.startsWith(u'.');
}

} // namespace Fernanda::FileTypes
