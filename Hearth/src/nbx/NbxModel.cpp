/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "nbx/NbxModel.h"

#include <QDomElement>
#include <QFont>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStyle>
#include <QVariant>

#include "core/Application.h"
#include "core/Debug.h"
#include "nbx/Nbx.h"
#include "nbx/NbxModelIcons.h"

namespace Hearth {

using namespace Qt::StringLiterals;

// TODO: Tooltip with metadata on file/folder
QVariant FnxModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    auto element = elementAt_(index);
    if (element.isNull()) return {};

    switch (role) {
    case Qt::DisplayRole: {
        auto display_name = Nbx::Xml::name(element);

        if (Nbx::Xml::isFile(element) && Nbx::Xml::isEdited(element)) {
            display_name.prepend(u"* "_s);
        }
        if (Nbx::Xml::hasEditedDescendant(element)) {
            display_name += u" (*)"_s;
        }

        return display_name;
    }

    case Qt::EditRole:
        return Nbx::Xml::name(element);

    case Qt::FontRole: {
        if (Nbx::Xml::isFile(element) && Nbx::Xml::isEdited(element)) {
            QFont font{};
            font.setItalic(true);
            return font;
        }

        return {};
    }

        // Icon is based on extension, not the "true" file type. If someone
        // imports a text file with a .pdf extension, it will show the PDF icon
        // here. When opened, Hearth handles it correctly: it would fail a
        // magic byte check and fall through to plain text
    case Qt::DecorationRole: {
        if (Nbx::Xml::isVirtualFolder(element)) {
            return FnxModelIcons::folder();

        } else if (Nbx::Xml::isFile(element)) {
            auto type = Files::fromPath(Nbx::Xml::relPath(element));
            return FnxModelIcons::file(type);
        }

        UNREACHABLE("FnxModel::data Qt::DecorationRole case");
        return {};
    }

    default:
        return {};
    }
}

} // namespace Hearth
