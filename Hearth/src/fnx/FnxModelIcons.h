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

#pragma once

#include <QHash>
#include <QIcon>
#include <QString>

#include "core/Files.h"

namespace Fernanda::FnxModelIcons {

using namespace Qt::StringLiterals;

namespace Internal {

    inline QString qrcPath_(Files::Type type)
    {
        switch (type) {
        case Files::Notebook:
            return u":/app-icons/Fernanda-32.png"_s;
        case Files::Markdown:
            return u":/breeze-icons/icons/mimetypes/32/text-x-markdown.svg"_s;
        case Files::Html:
            return u":/breeze-icons/icons/mimetypes/32/text-html.svg"_s;
        case Files::Pdf:
            return u":/breeze-icons/icons/mimetypes/32/application-pdf.svg"_s;
        case Files::Png:
            return u":/breeze-icons/icons/mimetypes/32/image-png.svg"_s;
        case Files::Jpeg:
            return u":/breeze-icons/icons/mimetypes/32/image-jpeg.svg"_s;
        case Files::Gif:
            return u":/breeze-icons/icons/mimetypes/32/image-gif.svg"_s;
        case Files::Tiff:
            return u":/breeze-icons/icons/mimetypes/32/image-tiff.svg"_s;
        case Files::Bmp:
            return u":/breeze-icons/icons/mimetypes/32/image-bmp.svg"_s;
        case Files::WebP:
            return u":/breeze-icons/icons/mimetypes/32/image-x-generic.svg"_s;
        default:
            return u":/breeze-icons/icons/mimetypes/32/text-x-generic.svg"_s;
        }

        // Re: Files::Type: Microsoft Word and RTF are converting imports only
        // in Notebook (will become plain text) - no icons needed; Fountain
        // doesn't have a dedicated icon; Notebook will open from within another
        // Notebook as plain text, but we can add an icon anyway
    }

} // namespace Internal

inline const QIcon& folder()
{
    static QIcon icon(u":/breeze-icons/icons/places/32/folder.svg"_s);
    return icon;
}

inline const QIcon& file(Files::Type type)
{
    static QHash<Files::Type, QIcon> cache{};

    if (auto it = cache.find(type); it != cache.end()) return it.value();
    return cache[type] = QIcon(Internal::qrcPath_(type));
}

} // namespace Fernanda::FnxModelIcons
