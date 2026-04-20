/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include "core/Tr.h"

namespace Fernanda::BetaAlert {

// Application-modal
inline void exec()
{
    QMessageBox box{};

    box.setWindowModality(Qt::ApplicationModal);
    box.setMinimumSize(400, 200);
    box.setIcon(QMessageBox::Warning);
    box.setTextInteractionFlags(Qt::NoTextInteraction);
    box.setText(Tr::nxBetaAlert());
    box.addButton(Tr::ok(), QMessageBox::AcceptRole);

    // TODO: Move to open/show
    box.exec();
}

} // namespace Fernanda::BetaAlert
