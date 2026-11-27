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

// The one authority on which files Fernanda imports, and as what. Extension-
// based, like FileTypes, and separate from it: FileTypes says what Fernanda
// opens, which the trees and the Vault rely on, and an importable file isn't
// one Fernanda opens. Importing reads a file from outside the vault and writes
// its text into the vault as a new file; the reading for each type is its own
// header's (Docx.h)
namespace Fernanda::ImportTypes {

using namespace Qt::StringLiterals;

enum class Type
{
    Unsupported = 0,
    Docx,
};

namespace Internal {

struct ExtensionEntry_
{
    Type type{};
    const char* extension = nullptr;
};

// Matched case-insensitively, so .DOCX and .docx are the same type. A type
// with several extensions takes one row for each
inline constexpr ExtensionEntry_ EXTENSIONS_[] = {
    { Type::Docx, ".docx" },
};

} // namespace Internal

// A path's type by its extension (std::filesystem semantics, via Coco::Path)
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

// Every type Fernanda imports, once each, in the table's order
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

// A type's extensions as a file dialog's patterns ("*.docx"), in the table's
// order. Empty for Unsupported
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
