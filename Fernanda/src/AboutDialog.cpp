/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QIcon>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <Qt>

#include "Tr.h"

#include "AboutDialog.h"
#include "Application.h"

namespace Fernanda::AboutDialog {

void exec()
{
    QMessageBox box{};

    box.setWindowModality(Qt::ApplicationModal);
    box.setIconPixmap(QIcon(":/icons/Fernanda.svg").pixmap(64, 64));
    box.setWindowTitle(Tr::Dialogs::aboutTitle());
    box.setText(Tr::Dialogs::aboutBody());
    box.setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    auto ok = box.addButton(Tr::Buttons::ok(), QMessageBox::AcceptRole);
    // auto licenses = box.addButton(Tr::Buttons::licenses(),
    // QMessageBox::AcceptRole);
    auto about_qt =
        box.addButton(Tr::Buttons::aboutQt(), QMessageBox::AcceptRole);

    auto a = app();
    a->connect(about_qt, &QPushButton::clicked, a, Application::aboutQt);

    box.setDefaultButton(ok);
    box.exec();
}

} // namespace Fernanda::AboutDialog
