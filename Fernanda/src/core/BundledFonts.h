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

#include <QFont>
#include <QString>
#include <QStringList>

#include <Coco/Path.h>

// NB: Keep in sync with External.qrc fonts! Doing all this programmatically on
// import would be maybe more trouble than it's worth

namespace Fernanda::BundledFonts {

using namespace Qt::StringLiterals;

constexpr auto EDITOR_MIN = 8;
constexpr auto EDITOR_MAX = 144;

inline Coco::PathList qrcPaths()
{
    return Coco::filePaths(
        { ":/courierprime/", ":/mononoki/", ":/opendyslexic/" },
        { "*.otf", "*.ttf" });
}

inline const QString& editorDefaultFamily()
{
    static const auto s = u"mononoki"_s;
    return s;
}

inline const QFont& editorDefault()
{
    static const QFont f(editorDefaultFamily(), 16, QFont::Bold, false);
    return f;
}

inline const QStringList& families()
{
    static const QStringList bundled = { "Courier Prime",
                                         "mononoki",
                                         "OpenDyslexic" };
    return bundled;
}

inline const QString& cssAtRules()
{
    static const auto s = uR"CSS(
@font-face {
    font-family: "Courier Prime";
    font-weight: normal;
    font-style: normal;
    src: url(qrc:/courierprime/Courier Prime.ttf) format("truetype");
}
@font-face {
    font-family: "Courier Prime";
    font-weight: bold;
    font-style: normal;
    src: url(qrc:/courierprime/Courier Prime Bold.ttf) format("truetype");
}
@font-face {
    font-family: "Courier Prime";
    font-weight: normal;
    font-style: italic;
    src: url(qrc:/courierprime/Courier Prime Italic.ttf) format("truetype");
}
@font-face {
    font-family: "Courier Prime";
    font-weight: bold;
    font-style: italic;
    src: url(qrc:/courierprime/Courier Prime Bold Italic.ttf) format("truetype");
}
@font-face {
    font-family: "mononoki";
    font-weight: normal;
    font-style: normal;
    src: url(qrc:/mononoki/mononoki-Regular.otf) format("opentype");
}
@font-face {
    font-family: "mononoki";
    font-weight: bold;
    font-style: normal;
    src: url(qrc:/mononoki/mononoki-Bold.otf) format("opentype");
}
@font-face {
    font-family: "mononoki";
    font-weight: normal;
    font-style: italic;
    src: url(qrc:/mononoki/mononoki-Italic.otf) format("opentype");
}
@font-face {
    font-family: "mononoki";
    font-weight: bold;
    font-style: italic;
    src: url(qrc:/mononoki/mononoki-BoldItalic.otf) format("opentype");
}
@font-face {
    font-family: "OpenDyslexic";
    font-weight: normal;
    font-style: normal;
    src: url(qrc:/opendyslexic/OpenDyslexic-Regular.otf) format("opentype");
}
@font-face {
    font-family: "OpenDyslexic";
    font-weight: bold;
    font-style: normal;
    src: url(qrc:/opendyslexic/OpenDyslexic-Bold.otf) format("opentype");
}
@font-face {
    font-family: "OpenDyslexic";
    font-weight: normal;
    font-style: italic;
    src: url(qrc:/opendyslexic/OpenDyslexic-Italic.otf) format("opentype");
}
@font-face {
    font-family: "OpenDyslexic";
    font-weight: bold;
    font-style: italic;
    src: url(qrc:/opendyslexic/OpenDyslexic-BoldItalic.otf) format("opentype");
}
)CSS"_s;

    return s;
}

} // namespace Fernanda::BundledFonts
