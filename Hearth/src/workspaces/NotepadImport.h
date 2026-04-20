/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QString>

#include <Coco/Path.h>

#include "core/Files.h"
#include "core/MagicBytes.h"
#include "workspaces/Docx.h"
#include "workspaces/Rtf.h"

namespace Hearth::NotepadImport {

struct Result
{
    QString text{};
    Files::Type type = Files::PlainText;
    QString suggestedName{};

    bool isValid() const { return !text.isEmpty(); }
};

/// TODO NF: Combine base conversion imports logic for this and NotebookImport
/// to use
inline Result process(const Coco::Path& path)
{
    auto name = path.stemQString();
    auto magic = MagicBytes::type(path, { MagicBytes::Zip, MagicBytes::Rtf });

    if (magic == MagicBytes::Zip
        && path.ext() == Files::canonicalExt(Files::MicrosoftWord)) {
        return { Docx::toPlainText(path), Files::PlainText, name };

    } else if (magic == MagicBytes::Rtf) {
        return { Rtf::toPlainText(path), Files::PlainText, name };
    }

    // Future conversion imports...

    return {};
}

inline QList<Result> process(const QList<Coco::Path>& paths)
{
    QList<Result> results{};

    for (const auto& path : paths) {
        auto result = process(path);
        if (result.isValid()) results << result;
    }

    return results;
}

} // namespace Hearth::NotepadImport
