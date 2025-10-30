/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QDomElement>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStyle>
#include <QVariant>
#include <Qt>

#include "Application.h"
#include "Fnx.h"
#include "FnxModel.h"

namespace Fernanda {

QVariant FnxModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    auto element = elementFromIndex_(index);

    if (role == Qt::DisplayRole) return element.attribute("name", "<unnamed>");

    if (role == Qt::DecorationRole) {
        if (element.tagName() == Fnx::XML_DIR_TAG) {
            return Application::style()->standardIcon(QStyle::SP_DirIcon);
        } else if (element.tagName() == Fnx::XML_FILE_TAG) {
            return Application::style()->standardIcon(QStyle::SP_FileIcon);
        }
    }

    // TODO: Qt::ToolTipRole for full path or UUID

    return {};
}

} // namespace Fernanda
