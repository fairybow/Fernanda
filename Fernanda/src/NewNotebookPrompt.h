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
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Coco/Path.h"

#include "Fnx.h"
#include "Tr.h"

// Application modal
namespace Fernanda::NewNotebookPrompt {

// TODO: Filter platform-forbidden characters!
inline QString exec()
{
    QDialog dialog{};
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle(Tr::nxNewNotebookTitle());
    dialog.setMinimumSize(240, 120);

    auto layout = new QVBoxLayout(&dialog);

    // Label
    auto label = new QLabel(Tr::nxNewNotebookBody(), &dialog);
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(label);

    // Input row ([QLineEdit][.fnx])
    auto input_layout = new QHBoxLayout;
    auto line_edit = new QLineEdit(&dialog);
    auto suffix_label = new QLabel(Fnx::Io::EXT, &dialog);
    suffix_label->setEnabled(false);
    input_layout->addWidget(line_edit, 1);
    input_layout->addWidget(suffix_label);
    layout->addLayout(input_layout);

    // Buttons
    auto button_box = new QDialogButtonBox(&dialog);
    auto ok = button_box->addButton(Tr::ok(), QDialogButtonBox::AcceptRole);
    auto cancel =
        button_box->addButton(Tr::cancel(), QDialogButtonBox::RejectRole);
    ok->setEnabled(false);
    cancel->setDefault(true);
    // Escape button behavior is automatic with RejectRole

    layout->addWidget(button_box);

    dialog.connect(
        line_edit,
        &QLineEdit::textChanged,
        [ok, cancel](const QString& text) {
            auto ok_enabled = !text.trimmed().isEmpty();
            ok->setEnabled(ok_enabled);
            ok->setDefault(ok_enabled);
            cancel->setDefault(!ok_enabled);
        });

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
    if (dialog.exec() == QDialog::Accepted) return line_edit->text().trimmed();

    return {};
}

// For now, let's allow this. The Notebook will be unsaved and the user will be
// warned against saving over an extant Notebook at startDir / name.fnx when
// Save As-ing

// inline void showExistsError(const Coco::Path& extantFnxPath)
//{
//     QMessageBox box{};
//     Internal::setCommonProperties_(box);
//     box.setTextInteractionFlags(Qt::TextSelectableByMouse);
//     box.setIcon(QMessageBox::Warning);
//
//     box.setText(
//         Tr::nxNewNotebookExistsErrBodyFormat().arg(extantFnxPath.toQString()));
//
//     // QMessageBox should handle platform-specific button position (left or
//     // right, for example) automatically
//     auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
//     box.setDefaultButton(ok);
//     box.setEscapeButton(ok);
//
//     // TODO: Move to open/show
//     box.exec();
// }

} // namespace Fernanda::NewNotebookPrompt
