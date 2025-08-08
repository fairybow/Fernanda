#pragma once

#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <Qt>
#include <QWidget>

#include "Tr.h"

namespace Dialogs::Unsaved
{
    enum Result { Cancel = 0, Save, Discard };

    inline Result exec(QWidget* parent)
    {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(Tr::Dialogs::Unsaved::title());
        box.setText(Tr::Dialogs::Unsaved::body());
        box.setTextInteractionFlags(Qt::NoTextInteraction);

        auto save = box.addButton(Tr::Buttons::save(), QMessageBox::AcceptRole);
        auto discard = box.addButton(Tr::Buttons::discard(), QMessageBox::DestructiveRole);
        auto cancel = box.addButton(Tr::Buttons::cancel(), QMessageBox::RejectRole);
        box.setDefaultButton(save);

        box.exec(); // Blocking
        auto clicked = box.clickedButton();

        if (clicked == save) return Save;
        if (clicked == discard) return Discard;
        return Cancel;
    }
}
