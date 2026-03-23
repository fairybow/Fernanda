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
