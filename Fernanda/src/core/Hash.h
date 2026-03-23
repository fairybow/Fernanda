/*
 * Fernanda is a plain text editor for fiction writing
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
#include <QCryptographicHash>
#include <QString>

#include <Coco/Path.h>

namespace Fernanda::Hash {

// Truncated SHA-256 hex of the full path. Used to group files by source in flat
// directories (backups, recovery entries)
//
// TODO: Length arg (up to max possible)
// TODO: Or name shortId or something
inline QString fromPath(const Coco::Path& path)
{
    return QCryptographicHash::hash(
               path.toQString().toUtf8(),
               QCryptographicHash::Sha256)
        .toHex()
        .left(8);
}

} // namespace Fernanda::Hash
