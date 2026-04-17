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

#include <array>
#include <utility>

#include <QChar>
#include <QString>
#include <QStringView>

#include "fmt/ToQString.h"

namespace Fernanda::Fmt {

using namespace Qt::StringLiterals;

namespace Internal {

    // Expand each arg to a QString once (NB: QString args pass through via the
    // implicit-share overload in ToQString.h, no copy)
    template <typename... Args>
    inline std::array<QString, sizeof...(Args)> expand_(Args&&... args)
    {
        return { toQString(std::forward<Args>(args))... };
    }

    // Estimated final size, minimizes reallocations during the walk
    inline qsizetype
    estimateSize_(QStringView tmpl, const QString* values, qsizetype count)
    {
        auto total = tmpl.size();

        for (qsizetype i = 0; i < count; ++i) {
            total += values[i].size();
        }

        return total;
    }

    // Single-pass walker. Copies literal runs in bulk via append(QStringView)
    // and substitutes args at each {}. Supports {{ and }} as literal braces
    inline QString
    format_(QStringView tmpl, const QString* values, qsizetype count)
    {
        QString out{};
        out.reserve(estimateSize_(tmpl, values, count));

        auto data = tmpl.constData();
        auto size = tmpl.size();
        qsizetype run_start = 0;
        qsizetype next_arg = 0;
        qsizetype i = 0;

        while (i < size) {
            auto ch = data[i];

            if (ch == u'{') {
                // Flush the literal run before the brace
                if (i > run_start) {
                    out.append(QStringView(data + run_start, i - run_start));
                }

                // Escaped '{{' -> literal '{'
                if (i + 1 < size && data[i + 1] == u'{') {
                    out.append(u'{');
                    i += 2;
                    run_start = i;

                    continue;
                }

                // Substitution '{}' (anything else is malformed, and we treat a
                // lone '{' as a literal)
                if (i + 1 < size && data[i + 1] == u'}') {
                    if (next_arg < count) {
                        out.append(values[next_arg]);
                        ++next_arg;
                    }

                    // If we're out of args, the '{}' is silently dropped

                    i += 2;
                    run_start = i;

                    continue;
                }

                // Malformed (treat the '{' as literal, keep walking)
                ++i;
                continue;
            }

            if (ch == u'}') {
                // Only '}}' is meaningful outside a substitution (a lone '}' is
                // treated as literal)
                if (i + 1 < size && data[i + 1] == u'}') {
                    if (i > run_start) {
                        out.append(
                            QStringView(data + run_start, i - run_start));
                    }

                    out.append(u'}');
                    i += 2;
                    run_start = i;

                    continue;
                }
            }

            ++i;
        }

        // Flush trailing literal
        if (run_start < size) {
            out.append(QStringView(data + run_start, size - run_start));
        }

        return out;
    }

} // namespace Internal

// Zero-arg fast path (no allocation, no walk)
inline QString format(QStringView tmpl) { return tmpl.toString(); }

template <typename... Args>
inline QString format(QStringView tmpl, Args&&... args)
{
    auto values = Internal::expand_(std::forward<Args>(args)...);
    return Internal::format_(tmpl, values.data(), values.size());
}

} // namespace Fernanda::Fmt
