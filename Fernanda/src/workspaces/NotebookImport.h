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

#include <QByteArray>
#include <QList>
#include <QString>

#include <Coco/Path.h>

#include "core/Files.h"
#include "core/Io.h"
#include "core/MagicBytes.h"
#include "workspaces/Docx.h"

namespace Fernanda::NotebookImport {

struct Result
{
    QByteArray content{};
    Files::Type type = Files::PlainText;
    QString suggestedName{};
    QString ext{};
};

inline Result process(const Coco::Path& path)
{
    auto name = path.stemQString();

    // Convertible types (extension + magic bytes)
    if (Files::isDocxFile(path)) {
        return { Docx::toPlainText(path).toUtf8(),
                 Files::PlainText,
                 name,
                 Files::canonicalExt(Files::PlainText) };
    }

    // Future: RTF, etc.

    // Passthrough: keep original extension, two-tier identification
    auto ext = path.extQString();

    auto mb = MagicBytes::type(path);
    if (mb != MagicBytes::NoKnownSignature) {
        return { Io::read(path), Files::fromMagicBytes(mb), name, ext };
    }

    return { Io::read(path), Files::fromPath(path), name, ext };
}

inline QList<Result> process(const QList<Coco::Path>& paths)
{
    QList<Result> results{};

    for (const auto& path : paths) {
        results << process(path);
    }

    return results;
}

} // namespace Fernanda::NotebookImport
