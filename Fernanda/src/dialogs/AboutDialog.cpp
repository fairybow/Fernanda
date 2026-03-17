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

#include "dialogs/AboutDialog.h"

#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>

#include "core/Application.h"
#include "core/Tr.h"

namespace Fernanda::AboutDialog {

void exec()
{
    QMessageBox box{};

    box.setWindowModality(Qt::ApplicationModal);
    box.setIconPixmap(QPixmap(":/icons/Fernanda-64.png"));
    box.setWindowTitle(Tr::nxAboutTitle());
    box.setText(Tr::nxAboutBody());
    box.setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
    // auto licenses = box.addButton(Tr::licenses(),
    // QMessageBox::AcceptRole);
    auto about_qt = box.addButton(Tr::aboutQt(), QMessageBox::AcceptRole);

    box.connect(about_qt, &QPushButton::clicked, app(), Application::aboutQt);

    box.setDefaultButton(ok);
    box.setEscapeButton(ok);

    // TODO: Move to open/show
    box.exec();
}

} // namespace Fernanda::AboutDialog
