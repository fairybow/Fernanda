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

#include <QByteArray>
#include <QString>

#include <Coco/Path.h>

#include "core/Io.h"

namespace Fernanda::Rtf {

/// TODO NF: Review this!
inline QString toPlainText(const Coco::Path& path)
{
    auto raw = Io::read(path);
    if (!raw.startsWith("{\\rtf")) return {};

    QString result{};
    qsizetype i = 0;
    auto len = raw.size();
    auto skip_depth = -1; // brace depth at which we entered a skippable group
    auto depth = 0;

    while (i < len) {
        auto ch = raw.at(i);

        if (ch == '{') {
            ++depth;

            // Check for \* (ignorable destination)
            if (i + 1 < len && raw.at(i + 1) == '\\') {
                // Peek for known skip groups or \*
                auto rest = QByteArray::fromRawData(
                    raw.constData() + i,
                    qMin(len - i, qsizetype(32)));
                if (rest.startsWith("{\\*") || rest.startsWith("{\\fonttbl")
                    || rest.startsWith("{\\colortbl")
                    || rest.startsWith("{\\stylesheet")
                    || rest.startsWith("{\\info")) {
                    if (skip_depth < 0) skip_depth = depth;
                }
            }

            ++i;
            continue;
        }

        if (ch == '}') {
            if (depth == skip_depth) skip_depth = -1;
            --depth;
            ++i;
            continue;
        }

        if (skip_depth >= 0) {
            ++i;
            continue;
        }

        if (ch == '\\') {
            ++i;
            if (i >= len) break;

            auto next = raw.at(i);

            // Hex escape: \'XX
            if (next == '\'') {
                if (i + 2 < len) {
                    auto hex =
                        QByteArray::fromRawData(raw.constData() + i + 1, 2);
                    bool ok = false;
                    auto code = hex.toUInt(&ok, 16);
                    if (ok) result += QChar(code);
                    i += 3;
                }
                continue;
            }

            // Control word
            if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')) {
                QByteArray word{};
                while (i < len
                       && ((raw.at(i) >= 'a' && raw.at(i) <= 'z')
                           || (raw.at(i) >= 'A' && raw.at(i) <= 'Z'))) {
                    word += raw.at(i);
                    ++i;
                }

                // Skip optional numeric parameter
                if (i < len
                    && (raw.at(i) == '-'
                        || (raw.at(i) >= '0' && raw.at(i) <= '9'))) {
                    ++i;
                    while (i < len && raw.at(i) >= '0' && raw.at(i) <= '9')
                        ++i;
                }

                // Delimiter space consumed
                if (i < len && raw.at(i) == ' ') ++i;

                if (word == "par" || word == "line")
                    result += u'\n';
                else if (word == "tab")
                    result += u'\t';

                continue;
            }

            // Escaped literal: \\ \{ \}
            if (next == '\\' || next == '{' || next == '}') {
                result += QLatin1Char(next);
                ++i;
                continue;
            }

            // Other (e.g., \~ \- \_)
            if (next == '~') result += QChar::Nbsp;
            ++i;
            continue;
        }

        // Plain text
        if (ch == '\r' || ch == '\n') {
            ++i;
            continue; // RTF line breaks are decorative
        }

        result += QLatin1Char(ch);
        ++i;
    }

    if (result.endsWith(u'\n')) result.chop(1);
    return result;
}

} // namespace Fernanda::Rtf
