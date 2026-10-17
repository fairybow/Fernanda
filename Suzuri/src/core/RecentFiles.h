/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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

#include <QList>
#include <QtTypes>

#include "core/FileRef.h"

namespace Suzuri {

// The files a vault window and its pop-outs have shown most recently, newest
// first: Go to File lists them when nothing has been typed. A plain value.
// VaultWindow keeps one and records each file that becomes a window's active
// tab, and the list is saved with the window's workspace.
//
// A file appears once: recording one already in the list moves it to the
// front. The oldest falls off past MAX, as in Obsidian's quick switcher
class RecentFiles
{
public:
    static constexpr qsizetype MAX = 10;

    [[nodiscard]] const QList<FileRef>& files() const noexcept
    {
        return files_;
    }

    void record(const FileRef& fileRef)
    {
        if (!fileRef.vault || fileRef.relative.isEmpty()) {
            return;
        }

        files_.removeAll(fileRef);
        files_.prepend(fileRef);

        if (files_.size() > MAX) {
            files_.resize(MAX);
        }
    }

    // Replace the list, as when it is restored: the first is the newest. A
    // repeat keeps its first place; past MAX, the rest are dropped
    void setFiles(const QList<FileRef>& files)
    {
        files_.clear();

        for (auto it = files.crbegin(); it != files.crend(); ++it) {
            record(*it);
        }
    }

private:
    QList<FileRef> files_{};
};

} // namespace Suzuri
