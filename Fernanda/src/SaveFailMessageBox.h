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

    inline void setCommonProperties_(QMessageBox& box)
    {
        box.setMinimumSize(400, 200);

        auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
        box.setDefaultButton(ok);
        box.setEscapeButton(ok);
    }

} // namespace Internal

inline void exec(const QString& fileDisplayName, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    box.setText(Tr::nxSaveFailBoxBodyFormat().arg(fileDisplayName));
    box.exec();
}

inline void exec(const QStringList& fileDisplayNames, QWidget* parent = nullptr)
{
    if (fileDisplayNames.isEmpty()) return;

    // Delegate to single-file prompt
    if (fileDisplayNames.size() == 1) {
        exec(fileDisplayNames.first(), parent);
        return;
    }

    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    // auto list = "<ul><li>" + fileDisplayNames.join("</li><li>") +
    // "</li></ul>";
    auto list = QString::fromUtf8("\n\u2022 ")
                + fileDisplayNames.join(QString::fromUtf8("\n\u2022 "));
    box.setText(Tr::nxSaveFailBoxMultiBodyFormat().arg(list));
    box.exec();
}

} // namespace Fernanda::SaveFailMessageBox
