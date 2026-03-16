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
#include <QStringList>
#include <QWidget>

#include <Coco/Path.h>

#include "core/Tr.h"

// TODO: Display error(s) from FileService/Io
namespace Fernanda::SaveFailMessageBox {

namespace Internal {

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

inline void exec(const Coco::Path& path, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    box.setText(Tr::nxSaveFailBoxBodyFormat().arg(path.prettyQString()));

    // TODO: Move to open/show
    box.exec();
}

inline void exec(const Coco::PathList& paths, QWidget* parent = nullptr)
{
    if (paths.isEmpty()) return;

    // Delegate to single-file prompt
    if (paths.size() == 1) {
        // TODO: Move to open/show
        exec(paths.first(), parent);
        return;
    }

    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    auto bullet = QStringLiteral("\n\u2022 ");
    auto list = bullet + Coco::toPrettyQStringList(paths).join(bullet);
    box.setText(Tr::nxSaveFailBoxMultiBodyFormat().arg(list));

    // TODO: Move to open/show
    box.exec();
}

} // namespace Fernanda::SaveFailMessageBox
