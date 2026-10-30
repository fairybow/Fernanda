/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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

#include <Coco/Path.h>

namespace Fernanda {

// What a listing reports for one entry, and what a node is keyed by: a
// child is "the same" across re-lists only if name AND kind match
struct VaultTreeEntry
{
    Coco::Path name{};
    bool isDir = false;

    bool operator==(const VaultTreeEntry& other) const = default;
};

// Owns its children. Only folders are ever listed
struct VaultTreeNode
{
    VaultTreeNode(const VaultTreeEntry& nodeEntry, VaultTreeNode* parentNode)
        : entry(nodeEntry)
        , parent(parentNode)
    {
    }

    ~VaultTreeNode() { qDeleteAll(children); }

    VaultTreeNode(const VaultTreeNode&) = delete;
    VaultTreeNode& operator=(const VaultTreeNode&) = delete;

    VaultTreeEntry entry;
    VaultTreeNode* parent;
    bool listed = false;
    QList<VaultTreeNode*> children{};
};

} // namespace Fernanda
