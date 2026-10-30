/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QRegularExpression>
#include <QString>

// A branch name made acceptable to git as it's typed, the way GitHub Desktop
// does it: what git-check-ref-format refuses is replaced by a hyphen, and the
// hyphens and pluses a name would start with are dropped. Git still judges
// the name when the branch is made; this only keeps the common mistakes (a
// space, a colon) from reaching it.
//
// Replaced, each run of them by one hyphen: control characters and spaces,
// ~ ^ : ? * [ \ | " < >, the sequence @{, two dots or more, a dot at the start
// or the end, ".lock" at the end, and a slash at the end. The last few
// characters are allowed by git but not in a Windows file name, which a
// branch's ref is. Text outside ASCII is kept
namespace Fernanda::GitRefName {

using namespace Qt::StringLiterals;

[[nodiscard]] inline QString sanitized(const QString& name)
{
    static const QRegularExpression refused(
        u"[\\x{00}-\\x{20}\\x{7F}~^:?*\\[\\\\|\"<>]+"
        u"|@\\{|\\.\\.+|^\\.|\\.$|\\.lock$|/$"_s);
    static const QRegularExpression leading(u"^[-+]*"_s);

    auto result = name;
    result.replace(refused, u"-"_s);
    result.remove(leading);
    return result;
}

} // namespace Fernanda::GitRefName
