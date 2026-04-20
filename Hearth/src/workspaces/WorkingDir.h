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

#include <Coco/Path.h>

namespace Fernanda {

// TODO: Rename NotebookWorkingDir
class WorkingDir
{
public:
    WorkingDir(const Coco::Path& dirPath)
        : dir_(dirPath)
        , wasAdopted_(dir_.exists())
    {
        if (!wasAdopted_) Coco::mkdir(dir_);
    }

    bool isValid() const { return dir_.exists(); }
    bool wasAdopted() const { return wasAdopted_; }
    Coco::Path path() const { return dir_; }

    bool remove()
    {
        if (dir_.isEmpty() || !dir_.exists()) return false;
        return Coco::purge(dir_);
    }

private:
    Coco::Path dir_;
    bool wasAdopted_;
};

} // namespace Fernanda
