#pragma once

#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <Qt>

#include "Tr.h"

namespace Dialogs::About
{
    inline void exec(QWidget* parent = nullptr)
    {
        QMessageBox box(parent);

        box.setIconPixmap(QIcon(":/icons/Fernanda.svg").pixmap(64, 64));
        box.setWindowTitle(Tr::Dialogs::About::title());
        box.setText(Tr::Dialogs::About::body());
        box.setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        auto ok = box.addButton(Tr::Buttons::ok(), QMessageBox::AcceptRole);
        //auto licenses = box.addButton(Tr::Buttons::licenses(), QMessageBox::AcceptRole);
        auto about_qt = box.addButton(Tr::Buttons::aboutQt(), QMessageBox::AcceptRole);
        qApp->connect(about_qt, &QPushButton::clicked, qApp, QApplication::aboutQt);

        box.setDefaultButton(ok);
        box.exec(); // Blocking
    }
}
