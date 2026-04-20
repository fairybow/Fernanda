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

#pragma once

#include <QByteArray>
#include <QString>
#include <QXmlStreamReader>

#include <miniz.h>

#include <Coco/Path.h>

namespace Hearth::Docx {

inline QString toPlainText(const Coco::Path& path)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, path.toString().c_str(), 0)) {
        return {};
    }

    auto cleanup = qScopeGuard([&] { mz_zip_reader_end(&zip); });

    constexpr auto entry = "word/document.xml";
    size_t size = 0;
    auto data = mz_zip_reader_extract_file_to_heap(&zip, entry, &size, 0);
    if (!data) return {};

    QByteArray xml(
        static_cast<const char*>(data),
        static_cast<qsizetype>(size));
    mz_free(data);

    QString result{};
    QXmlStreamReader reader(xml);
    auto in_paragraph = false;

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            if (reader.name() == u"p") in_paragraph = true;

        } else if (reader.isEndElement()) {
            if (reader.name() == u"p") {
                if (in_paragraph) {
                    result += u'\n';
                    in_paragraph = false;
                }
            }

        } else if (reader.isCharacters() && in_paragraph) {
            result += reader.text();
        }
    }

    if (result.endsWith(u'\n')) result.chop(1);
    return result;
}

} // namespace Hearth::Docx
