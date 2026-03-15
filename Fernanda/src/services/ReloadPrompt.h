/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <Coco/Bool.h>

#include "core/Tr.h"

// Window-modal dialog for prompting the user when a file has been modified
// externally. Returns true if the user wants to reload from disk, false if they
// want to keep their in-memory version
namespace Fernanda::ReloadPrompt {

COCO_BOOL(Reload);

/// 1. the 3 prompts take paths/path lists
/// 2. the call sites reworked - just give paths or composite, prospective paths if off disk
/// 3. call sites in Notepad, ViewService, and Notebook (just the FNX there)
/// 4. format for display in the prompts/dialogs

inline Reload exec(const QString& displayPath, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    box.setWindowModality(Qt::WindowModal);
    box.setMinimumSize(400, 200);
    box.setTextInteractionFlags(Qt::NoTextInteraction);

    box.setText(Tr::nxReloadPromptBodyFormat().arg(displayPath));

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

} // namespace Fernanda::ReloadPrompt
