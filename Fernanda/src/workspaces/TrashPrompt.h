/*
 * Fernanda is a plain text editor for fiction writing
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "core/Tr.h"

namespace Fernanda::TrashPrompt {

inline bool exec(int count, QWidget* parent = nullptr)
{
    QDialog dialog(parent);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumSize(240, 100);

    auto layout = new QVBoxLayout(&dialog);

    // Label
    auto label = new QLabel(Tr::nbTrashPromptBody(count), &dialog);
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(label);

    // Buttons
    auto button_box = new QDialogButtonBox(&dialog);
    button_box->addButton(Tr::ok(), QDialogButtonBox::AcceptRole);
    auto cancel =
        button_box->addButton(Tr::cancel(), QDialogButtonBox::RejectRole);
    cancel->setDefault(true);
    // Escape button behavior is automatic with RejectRole

    layout->addWidget(button_box);

    dialog.connect(
        button_box,
        &QDialogButtonBox::accepted,
        &dialog,
        &QDialog::accept);

    dialog.connect(
        button_box,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject);

    // TODO: Move to open/show
    return (dialog.exec() == QDialog::Accepted);
}

} // namespace Fernanda::TrashPrompt
