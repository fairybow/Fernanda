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

#include <QRandomGenerator>
#include <QString>

namespace Hearth::Random {

inline QString token(int length)
{
    constexpr auto chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    constexpr auto count = 62;

    auto len = qMax(1, length);
    QString result{};
    result.reserve(len);

    for (auto i = 0; i < len; ++i)
        result += chars[QRandomGenerator::global()->bounded(count)];

    return result;
}

} // namespace Hearth::Random
