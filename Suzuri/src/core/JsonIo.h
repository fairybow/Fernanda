/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Io.h"

// The JSON layer over Fernanda::Io. JsonIo reads and serializes; Io moves the
// bytes. Every config file (fernanda.json, settings.json, workspace.json) is a
// JSON *object*, so the API is object-in / object-out — QJsonDocument stays an
// internal detail. Stateless free functions, like Io itself.
//
// Reuses Io::CreateDirs rather than defining a second bool type: config files
// live under dirs that may not exist yet (.fernanda/, appData()), so the write
// forwards the same dir-ensuring behavior Io already has.
namespace Fernanda::JsonIo {

// Returns the root object, or an empty object on any failure — a missing file
// (normal on first run), a read error, malformed JSON, or a non-object root.
// Callers treat {} as "no data, fall to defaults"; there's no exception and no
// optional, mirroring Io::read returning an empty QByteArray. Io has already
// logged an empty/missing/unreadable path, so those stay quiet here; only
// genuinely corrupt content — bytes that exist but don't parse — WARNs, since
// that's the case worth surfacing
inline QJsonObject read(const Coco::Path& path)
{
    auto data = Io::read(path);

    // Missing or empty file: not an error. Io logged it if it mattered; a
    // first-run config simply isn't there yet
    if (data.isEmpty()) {
        return {};
    }

    QJsonParseError parse_error{};
    auto document = QJsonDocument::fromJson(data, &parse_error);

    if (parse_error.error != QJsonParseError::NoError) {
        WARN(
            "Malformed JSON in {} (Error: {})!",
            path,
            parse_error.errorString());
        return {};
    }

    if (!document.isObject()) {
        WARN("JSON root in {} is not an object!", path.prettyQString());
        return {};
    }

    return document.object();
}

// Serialize the object and hand the bytes to Io::write (atomic via QSaveFile).
// Indented so the committed vault files stay diffable and hand-editable;
// harmless for the machine-local fernanda.json. Returns Io::write's result — it
// already logs a failed write, and leaves any existing file untouched on a
// failed commit
inline bool write(
    const QJsonObject& object,
    const Coco::Path& path,
    Io::CreateDirs createDirs)
{
    auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    return Io::write(bytes, path, createDirs);
}

} // namespace Fernanda::JsonIo
