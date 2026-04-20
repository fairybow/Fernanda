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

#include <QByteArray>
#include <QList>
#include <QString>

#include <Coco/Path.h>

#include "core/Files.h"
#include "core/Io.h"
#include "core/MagicBytes.h"
#include "workspaces/Docx.h"
#include "workspaces/Rtf.h"

namespace Hearth::NotebookImport {

struct Result
{
    QByteArray content{};
    Files::Type type = Files::PlainText;
    QString suggestedName{};
    QString ext{};
};

/// TODO NF: I don't like that each case in the if-else-if is almost the same.
/// Currently, we only import 2 file types with conversion, both to plain text,
/// and that may or may not be the only kind of conversion we ever do. Unsure...
inline Result process(const Coco::Path& path)
{
    auto name = path.stemQString();
    auto magic = MagicBytes::type(path);

    // Convertible types (extension + magic bytes)
    if (magic == MagicBytes::Zip
        && path.ext() == Files::canonicalExt(Files::MicrosoftWord)) {
        return { Docx::toPlainText(path).toUtf8(),
                 Files::PlainText,
                 name,
                 Files::canonicalExt(Files::PlainText) };
    } else if (magic == MagicBytes::Rtf) {
        return { Rtf::toPlainText(path).toUtf8(),
                 Files::PlainText,
                 name,
                 Files::canonicalExt(Files::PlainText) };
    }

    // Future conversion imports...

    // Passthrough: keep original extension, two-tier identification
    auto ext = path.extQString();

    if (magic != MagicBytes::NoKnownSignature) {
        return { Io::read(path), Files::fromMagicBytes(magic), name, ext };
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

} // namespace Hearth::NotebookImport
