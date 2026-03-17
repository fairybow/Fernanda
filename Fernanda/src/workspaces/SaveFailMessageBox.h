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
