/*
 * Fernanda  Copyright (C) 2025  fairybow
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
#include <QStringList>
#include <QWidget>
#include <Qt>

#include "Tr.h"

// TODO: Display error(s) from FileService/Io
namespace Fernanda::SaveFailMessageBox {

namespace Internal {

    constexpr auto BULLET_ = "\n\u2022 ";

    inline void setCommonProperties_(QMessageBox& box)
    {
        box.setWindowModality(Qt::WindowModal);
        box.setMinimumSize(400, 200);

        auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
        box.setDefaultButton(ok);
        box.setEscapeButton(ok);
        box.setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

} // namespace Internal

inline void exec(const QString& fileDisplayName, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    box.setText(Tr::nxSaveFailBoxBodyFormat().arg(fileDisplayName));

    // TODO: Move to open/show
    box.exec();
}

inline void exec(const QStringList& fileDisplayNames, QWidget* parent = nullptr)
{
    if (fileDisplayNames.isEmpty()) return;

    // Delegate to single-file prompt
    if (fileDisplayNames.size() == 1) {
        // TODO: Move to open/show
        exec(fileDisplayNames.first(), parent);
        return;
    }

    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    auto list = QString::fromUtf8(Internal::BULLET_)
                + fileDisplayNames.join(QString::fromUtf8(Internal::BULLET_));
    box.setText(Tr::nxSaveFailBoxMultiBodyFormat().arg(list));

    // TODO: Move to open/show
    box.exec();
}

} // namespace Fernanda::SaveFailMessageBox
