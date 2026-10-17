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
#include <QMimeData>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

namespace Fernanda::Ui {

// The drag-and-drop contract for vault entries (files or folders) moved within,
// or opened from, the file tree. One private MIME type, shared by the drag
// source (VaultTreeView) and every surface that accepts the drop — the file
// tree itself (a move) and an editor pane (an open). Private on purpose:
// nothing outside Suzuri accepts it, so an Explorer/Desktop drop is a silent
// no-op, and folders can't leave the app at all. Dragging a file out to the OS
// as a link-like object, if ever built, would add a SECOND, external format
// alongside this one rather than changing it.
//
// Payload: one ABSOLUTE path per line, UTF-8. No pointer or index is
// serialized, so it crosses windows (into a pop-out) with no type coupling to
// the source widget. It is list-shaped although a drag carries exactly one
// entry at a time — Obsidian's explorer drags multiple, and shaping the payload
// for it now means that feature never has to version the format. Receivers that
// only handle one entry check the count and refuse the rest
inline constexpr auto VAULT_ENTRY_DRAG_MIME_TYPE =
    "application/x-fernanda-vault-entry";

// The mime data for a drag of these entries. Caller passes it to
// QDrag::setMimeData, which takes ownership
[[nodiscard]] inline QMimeData*
toVaultEntryDragMime(const Coco::PathList& absolutes)
{
    QStringList lines{};
    for (const auto& absolute : absolutes) {
        lines << absolute.toQString();
    }

    auto* mime = new QMimeData;
    mime->setData(
        QString::fromLatin1(VAULT_ENTRY_DRAG_MIME_TYPE),
        lines.join(u'\n').toUtf8());

    return mime;
}

// The entries a drag carries — empty for any drag that isn't ours (a foreign
// or OS drag has no such format), which is what every caller's first check
// keys on
[[nodiscard]] inline Coco::PathList
fromVaultEntryDragMime(const QMimeData* mime)
{
    auto format = QString::fromLatin1(VAULT_ENTRY_DRAG_MIME_TYPE);
    if (!mime || !mime->hasFormat(format)) {
        return {};
    }

    Coco::PathList out{};
    for (const auto& line : QString::fromUtf8(mime->data(format))
                                .split(u'\n', Qt::SkipEmptyParts)) {
        out << Coco::Path(line);
    }

    return out;
}

} // namespace Fernanda::Ui
