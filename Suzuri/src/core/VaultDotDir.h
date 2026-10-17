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
#include <QString>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/CoreConstants.h"
#include "core/Io.h"
#include "core/Publication.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

[[nodiscard]] inline Coco::Path vaultDotDir(const Coco::Path& root)
{
    return root / (u"."_s + PUB_APP_NAME_LOWER_QSTRING);
}

// Seed the ignore ONCE, at first creation — keyed on .suzuri/'s absence, not
// the .gitignore's. If the dot-folder already exists the vault is established
// and its contents are the user's: a .gitignore they deleted stays deleted,
// never rewritten behind their back. The write also makes the folder
// (Io::write, CreateDirs::Yes), so "the folder is absent" IS "this is initial
// creation" — no separate first-run flag needed. A .suzuri/ deleted while the
// vault is open is recreated at the next workspace save, as Obsidian does with
// .obsidian.
//
// It never creates the vault root itself. Every open path has already checked
// the root exists, so a missing root here means the vault folder was deleted or
// moved outside Suzuri mid-session — and reading .suzuri/'s absence as "new
// vault" would recreate the root, leaving a ghost folder where the vault was.
// Returns whether .suzuri/ exists afterward, so a caller about to write into it
// (WorkspaceFile) can skip instead
inline bool ensureVaultDotDir(const Coco::Path& root)
{
    if (!root.isDir()) {
        WARN("Vault folder {} is gone — not recreating it!", root);
        return false;
    }

    auto dot_dir = vaultDotDir(root);
    if (dot_dir.exists()) {
        return true;
    }

    // Makes only .suzuri/ itself: the root was checked above
    Io::write(
        QByteArray(WORKSPACE_FILE_NAME) + '\n',
        dot_dir / u".gitignore"_s,
        Io::CreateDirs::Yes);

    return dot_dir.exists();
}

} // namespace Fernanda
