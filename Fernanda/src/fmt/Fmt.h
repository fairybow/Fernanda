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
#include <QLatin1StringView>
#include <QString>
#include <QStringView>

#include "fmt/ToQString.h"

namespace Fernanda::Fmt {

using namespace Qt::StringLiterals;

namespace Internal {

    // Per-arg wrapper that either borrows (QString/QStringView paths, zero
    // alloc) or owns (fallback path, one alloc via toQString). `view` is the
    // canonical appendable form. `owned` is the backing store when we have to
    // materialize one
    //
    // Non-movable and non-copyable. `view` may point into this object's own
    // `owned` member, so relocation would invalidate it. Elements are
    // constructed in place in the std::array below via braced aggregate init,
    // which is guaranteed to avoid copies/moves in C++20
    struct ArgView_
    {
        QStringView view;
        QString owned;

        ArgView_() = default;

        ArgView_(QStringView v)
            : view(v)
        {
        }

        ArgView_(QString&& s)
            : owned(std::move(s))
        {
            view = owned;
        }

        ArgView_(const ArgView_&) = delete;
        ArgView_& operator=(const ArgView_&) = delete;
        ArgView_(ArgView_&&) = delete;
        ArgView_& operator=(ArgView_&&) = delete;

        qsizetype size() const noexcept { return view.size(); }
        void appendTo(QString& out) const { out.append(view); }
    };

    // --- Borrowing builders (zero-alloc) ---

    inline ArgView_ makeArg_(const QString& s)
    {
        return ArgView_(QStringView(s));
    }
    inline ArgView_ makeArg_(QStringView s) { return ArgView_(s); }

    // --- Owning builders (one alloc) ---

    inline ArgView_ makeArg_(QLatin1StringView s)
    {
        return ArgView_(s.toString());
    }
    inline ArgView_ makeArg_(const char* s)
    {
        return ArgView_(QString::fromUtf8(s));
    }

    // Generic fallback (anything ToQString.h knows how to handle)
    template <typename T> inline ArgView_ makeArg_(T&& value)
    {
        return ArgView_(toQString(std::forward<T>(value)));
    }

    // Expand each arg to an ArgView_. QString/QStringView args take the
    // zero-alloc path (everything else routes through toQString)
    template <typename... Args>
    inline std::array<ArgView_, sizeof...(Args)> expand_(Args&&... args)
    {
        return { makeArg_(std::forward<Args>(args))... };
    }

    // Estimated final size, minimizes reallocations during the walk
    inline qsizetype
    estimateSize_(QStringView tmpl, const ArgView_* values, qsizetype count)
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
    format_(QStringView tmpl, const ArgView_* values, qsizetype count)
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
                        values[next_arg].appendTo(out);
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
