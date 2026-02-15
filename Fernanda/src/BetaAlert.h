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

#include "Tr.h"

namespace Fernanda::BetaAlert {

// Application-modal
inline void exec()
{
    QMessageBox box;
    box.setWindowModality(Qt::ApplicationModal);
    box.setMinimumSize(400, 200);
    box.setIcon(QMessageBox::Warning);
    box.setTextInteractionFlags(Qt::NoTextInteraction);
    box.setText(Tr::nxBetaAlert());

    auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);

    // TODO: Move to open/show
    box.exec();
}

} // namespace Fernanda::BetaAlert
