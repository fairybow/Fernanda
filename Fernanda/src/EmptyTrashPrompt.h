/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Tr.h"

namespace Fernanda::EmptyTrashPrompt {

inline bool exec(QWidget* parent = nullptr)
{
    QDialog dialog(parent);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumSize(240, 120);

    auto layout = new QVBoxLayout(&dialog);

    // Label
    auto label = new QLabel(Tr::nbEmptyTrashPromptBody(), &dialog);
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

} // namespace Fernanda::EmptyTrashPrompt
