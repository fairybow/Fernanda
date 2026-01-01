/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "AboutDialog.h"

#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <Qt>

#include "Application.h"
#include "Tr.h"

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
