/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "fnx/FnxModel.h"

#include <QDomElement>
#include <QFont>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStyle>
#include <QVariant>

#include "core/Application.h"
#include "fnx/Fnx.h"

namespace Fernanda {

/// TODO FT: Can check meta file type for tree view icon somehow? Route through
/// Fnx::Xml?
QVariant FnxModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    auto element = elementAt_(index);
    if (element.isNull()) return {};

    switch (role) {
    case Qt::DisplayRole: {
        auto display_name = Fnx::Xml::name(element);

        if (Fnx::Xml::isFile(element) && Fnx::Xml::isEdited(element))
            display_name.prepend(QStringLiteral("* "));
        if (Fnx::Xml::hasEditedDescendant(element))
            display_name += QStringLiteral(" (*)");

        return display_name;
    }

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
