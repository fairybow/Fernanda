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

#include <QString>
#include <QStringView>

namespace Fernanda::MarkupWebcode {

using namespace Qt::StringLiterals;

inline QString
htmlDoc(const QString& fontFaceKit, QStringView css, const QString& body)
{
    static const auto s = uR"(
<!DOCTYPE html>
<html>
    <head>
        <style>%1%2</style>
    </head>
    <body>
        %3
    </body>
</html>
)"_s;

    return s.arg(fontFaceKit, css, body);
}

inline QString jsOuterHtml(int index, const QString& escaped)
{
    static const auto s = uR"JS(
document.querySelector("[data-idx='%1']").outerHTML = `%2`;
)JS"_s;

    return s.arg(index).arg(escaped);
}

inline QString jsPatchHtmlBody(const QString& statements)
{
    // Double RAF (See:
    // https://stackoverflow.com/questions/44145740/how-does-double-requestanimationframe-work)
    static const auto s = uR"JS(
var lastScrollY = window.scrollY;
%1
requestAnimationFrame(function() {
    requestAnimationFrame(function() {
        window.scrollTo(0, lastScrollY);
    });
});
)JS"_s;

    return s.arg(statements);
}

inline QString jsReplaceHtmlBody(const QString& body)
{
    static const auto s = uR"JS(
var lastScrollY = window.scrollY;
document.body.innerHTML = `%1`;
requestAnimationFrame(function() {
    requestAnimationFrame(function() {
        window.scrollTo(0, lastScrollY);
    });
});
)JS"_s;

    return s.arg(body);
}

} // namespace Fernanda::MarkupWebcode
