/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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
#include <QList>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

#include "core/files/FileTypes.h"

// The one authority on which files Fernanda imports, and as what. A file is
// imported one of two ways: converted, its text read out and written into the
// vault as a new text file (the reading for each type is its own header's, as
// Docx.h), or copied in unchanged, for a file Fernanda already opens.
//
// Only converted types are listed here. Which files are copied is FileTypes'
// to say, so the two lists can't drift apart. Converted types stay out of
// FileTypes: it says what Fernanda opens, which the trees and the Vault rely
// on, and a converted file isn't one Fernanda opens
namespace Fernanda::ImportTypes {

using namespace Qt::StringLiterals;

enum class Type
{
    Unsupported = 0,
    Copy, // Any type Fernanda opens (FileTypes::isSupported)
    Docx,
};

namespace Internal {

struct ExtensionEntry_
{
    Type type{};
    const char* extension = nullptr;
};

// Converted types only. Matched case-insensitively, so .DOCX and .docx are
// the same type. A type with several extensions takes one row for each
inline constexpr ExtensionEntry_ EXTENSIONS_[] = {
    { Type::Docx, ".docx" },
};

} // namespace Internal

// A path's type by its extension (std::filesystem semantics, via Coco::Path):
// a converted type from the table, else Copy for a type Fernanda opens
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

    if (FileTypes::isSupported(path)) {
        return Type::Copy;
    }

    return Type::Unsupported;
}

// Every converted type, once each, in the table's order. Copy isn't one: the
// types it covers are FileTypes::types()
[[nodiscard]] inline QList<Type> types()
{
    QList<Type> result{};

    for (const auto& entry : Internal::EXTENSIONS_) {
        if (!result.contains(entry.type)) {
            result << entry.type;
        }
    }

    return result;
}

// A converted type's extensions as a file dialog's patterns ("*.docx"), in
// the table's order. Empty for Unsupported and Copy, whose patterns are
// FileTypes::patternsOf each of its types
[[nodiscard]] inline QStringList patternsOf(Type type)
{
    QStringList result{};

    for (const auto& entry : Internal::EXTENSIONS_) {
        if (entry.type == type) {
            result << u"*"_s + QString::fromLatin1(entry.extension);
        }
    }

    return result;
}

} // namespace Fernanda::ImportTypes
