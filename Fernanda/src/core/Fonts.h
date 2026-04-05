#pragma once

#include <QString>
#include <QStringList>

// Keep in sync with External.qrc fonts

namespace Fernanda::Fonts {

using namespace Qt::StringLiterals;

inline QStringList families()
{
    static const QStringList bundled = { "Courier Prime",
                                         "mononoki",
                                         "OpenDyslexic" };
    return bundled;
}

static QString cssAtRules()
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

} // namespace Fernanda::Fonts
