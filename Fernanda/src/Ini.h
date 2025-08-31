#pragma once

#include <QFont>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtMinMax>

#include "Commander.h"
#include "Utility.h"

namespace Fernanda::Ini {

//...

namespace EditorFont {

    constexpr auto FAMILY_KEY = "EditorsFont/Family";
    constexpr auto DEFAULT_FAMILY = "mononoki";

    constexpr auto PT_SIZE_KEY = "EditorsFont/Size";
    constexpr auto PT_SIZE_MIN = 8;
    constexpr auto PT_SIZE_MAX = 72;
    constexpr auto DEFAULT_PT_SIZE = 14;

    constexpr auto BOLD_KEY = "EditorsFont/Bold";
    constexpr auto DEFAULT_BOLD = false;

    constexpr auto ITALIC_KEY = "EditorsFont/Italic";
    constexpr auto DEFAULT_ITALIC = false;

    inline QFont load(Commander* commander)
    {
        QFont font{};
        if (!commander) return font;

        auto family = commander->query<QString>(
            Queries::Setting,
            { { "key", FAMILY_KEY }, { "default", DEFAULT_FAMILY } });

        auto size = commander->query<int>(
            Queries::Setting,
            { { "key", PT_SIZE_KEY }, { "default", DEFAULT_PT_SIZE } });

        auto is_bold = commander->query<bool>(
            Queries::Setting,
            { { "key", BOLD_KEY }, { "default", DEFAULT_BOLD } });

        auto is_italic = commander->query<bool>(
            Queries::Setting,
            { { "key", ITALIC_KEY }, { "default", DEFAULT_ITALIC } });

        font.setFamily(family);
        font.setPointSize(qBound(PT_SIZE_MIN, size, PT_SIZE_MAX));
        font.setBold(is_bold);
        font.setItalic(is_italic);

        return font;
    }

    // Old
    //inline void save(const QFont& font, Settings* settings)
    //{
    //    if (!settings || !settings->isWritable()) return;

    //    settings->beginGroup(Groups::EDITORS_FONT);
    //    settings->setValue(FAMILY_KEY, font.family());
    //    settings->setValue(PT_SIZE_KEY, font.pointSize());
    //    settings->setValue(BOLD_KEY, font.bold());
    //    settings->setValue(ITALIC_KEY, font.italic());
    //    settings->endGroup();
    //}

} // namespace EditorFont

} // namespace Fernanda::Ini
