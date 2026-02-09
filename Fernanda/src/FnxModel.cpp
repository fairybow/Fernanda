/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "FnxModel.h"

#include <QDomElement>
#include <QFont>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStyle>
#include <QVariant>
#include <Qt>

#include "Application.h"
#include "Fnx.h"

namespace Fernanda {

QVariant FnxModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    auto element = elementAt_(index);
    if (element.isNull()) return {};

    switch (role) {
    case Qt::DisplayRole:
        if (Fnx::Xml::isFile(element) && Fnx::Xml::isEdited(element))
            return Fnx::Xml::name(element) + QStringLiteral(" *");
        return Fnx::Xml::name(element);

    case Qt::EditRole:
        return Fnx::Xml::name(element);

    case Qt::FontRole: {
        if (Fnx::Xml::isFile(element) && Fnx::Xml::isEdited(element)) {
            QFont font{};
            font.setItalic(true);
            return font;
        }

        return {};
    }

    case Qt::DecorationRole: {
        if (Fnx::Xml::isVirtualFolder(element)) {

            if (cachedDirIcon_.isNull())
                cachedDirIcon_ =
                    Application::style()->standardIcon(QStyle::SP_DirIcon);

            return cachedDirIcon_;

        } else if (Fnx::Xml::isFile(element)) {

            if (cachedFileIcon_.isNull())
                cachedFileIcon_ =
                    Application::style()->standardIcon(QStyle::SP_FileIcon);

            return cachedFileIcon_;
        }

        return {}; // Unreachable
    }

    default:
        return {};
    }
}

} // namespace Fernanda
