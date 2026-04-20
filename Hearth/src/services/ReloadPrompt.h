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

#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <Coco/Bool.h>
#include <Coco/Path.h>

#include "core/Tr.h"

// Window-modal dialog for prompting the user when a file has been modified
// externally. Returns true if the user wants to reload from disk, false if they
// want to keep their in-memory version
namespace Hearth::ReloadPrompt {

COCO_BOOL(Reload)

inline Reload exec(const Coco::Path& path, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    box.setWindowModality(Qt::WindowModal);
    box.setMinimumSize(400, 200);
    box.setTextInteractionFlags(Qt::NoTextInteraction);

    box.setText(Tr::nxReloadPromptBodyFormat().arg(path.prettyQString()));

    // QMessageBox should handle platform-specific button ordering automatically
    auto reload =
        box.addButton(Tr::nxReloadPromptReload(), QMessageBox::AcceptRole);
    box.addButton(Tr::nxReloadPromptKeep(), QMessageBox::RejectRole);

    box.setDefaultButton(reload);
    // Escape button behavior is automatic with RejectRole

    // TODO: Move to open/show
    box.exec();

    return (box.clickedButton() == reload) ? Reload::Yes : Reload::No;
}

} // namespace Hearth::ReloadPrompt
