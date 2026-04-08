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

#include <QString>

#include <Coco/Path.h>

#include "core/Files.h"
#include "workspaces/Docx.h"

namespace Fernanda::NotepadImport {

struct Result
{
    QString text{};
    Files::Type type = Files::PlainText;
    QString suggestedName{};

    bool isValid() const { return !text.isEmpty(); }
};

inline Result process(const Coco::Path& path)
{
    auto name = path.stemQString();

    if (Files::isDocxFile(path)) {
        return { Docx::toPlainText(path), Files::PlainText, name };
    }

    // Future: RTF, etc.

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

} // namespace Fernanda::NotepadImport
