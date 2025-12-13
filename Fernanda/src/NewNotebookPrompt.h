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
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include "Tr.h"

namespace Fernanda::NewNotebookPrompt {

inline QString exec()
{
    /// TODO: QDialogButtonBox in other prompts, and also can translate like:
    /// button_box->button(QDialogButtonBox::Cancel)->setText(Tr::cancel());

    QDialog dialog{};
    dialog.setMinimumSize(400, 200);
    /*auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
    box.setDefaultButton(ok);
    box.setEscapeButton(ok);
    box.setText("text");*/
    dialog.exec();

    return {}; // temp
}

} // namespace Fernanda::NewNotebookPrompt
