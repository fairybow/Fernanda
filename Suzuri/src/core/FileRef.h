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

#include <Coco/Path.h>

namespace Suzuri {

class Vault;

// Identifies a file: a vault, and a path relative to that vault's root. A bare
// path isn't enough, since one window reaches two vaults. A plain value nobody
// owns; Vault::makeFileRef is the one place an absolute path becomes one
struct FileRef
{
    Vault* vault = nullptr; // a project vault, or the common one
    Coco::Path relative{};  // relative to that vault's root

    // The same file: the same vault, and the same path as written
    bool operator==(const FileRef&) const = default;
};

} // namespace Suzuri
