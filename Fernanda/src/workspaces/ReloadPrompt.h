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

#include "core/Tr.h"

// Window-modal dialog for prompting the user when a file has been modified
// externally. Returns true if the user wants to reload from disk, false if they
// want to keep their in-memory version.
namespace Fernanda::ReloadPrompt {

// TODO: Use named bool?
inline bool exec(const QString& fileDisplayName, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    box.setWindowModality(Qt::WindowModal);
    box.setMinimumSize(400, 200);
    box.setTextInteractionFlags(Qt::NoTextInteraction);

    box.setText(Tr::nxReloadPromptBodyFormat().arg(fileDisplayName));

    // QMessageBox should handle platform-specific button ordering automatically
    auto reload = box.addButton(Tr::reload(), QMessageBox::AcceptRole);
    box.addButton(Tr::keepMine(), QMessageBox::RejectRole);

    box.setDefaultButton(reload);
    // Escape button behavior is automatic with RejectRole

    // TODO: Move to open/show
    box.exec();

    return (box.clickedButton() == reload);
}

} // namespace Fernanda::ReloadPrompt
