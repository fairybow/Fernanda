#pragma once

#include <QFont>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtMinMax>

#include "Commander.h"
#include "Utility.h"

namespace Fernanda::Ini {

namespace Internal {

    template <typename T>
    inline T
    get(Commander* commander, const char* key, const QVariant& defaultValue)
    {
        return commander->query<T>(
            Queries::Setting,
            { { "key", key }, { "default", defaultValue } });
    }

    inline void
    set(Commander* commander, const char* key, const QVariant& value)
    {
        commander->execute(
            Commands::SetSetting,
            { { "key", key }, { "value", value } });
    }

} // namespace Internal

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

        auto family =
            Internal::get<QString>(commander, FAMILY_KEY, DEFAULT_FAMILY);
        auto size = Internal::get<int>(commander, PT_SIZE_KEY, DEFAULT_PT_SIZE);
        auto is_bold = Internal::get<bool>(commander, BOLD_KEY, DEFAULT_BOLD);
        auto is_italic =
            Internal::get<bool>(commander, ITALIC_KEY, DEFAULT_ITALIC);

        font.setFamily(family);
        font.setPointSize(qBound(PT_SIZE_MIN, size, PT_SIZE_MAX));
        font.setBold(is_bold);
        font.setItalic(is_italic);

        return font;
    }

    inline void save(const QFont& font, Commander* commander)
    {
        if (!commander) return;

        Internal::set(commander, FAMILY_KEY, font.family());
        Internal::set(commander, PT_SIZE_KEY, font.pointSize());
        Internal::set(commander, BOLD_KEY, font.bold());
        Internal::set(commander, ITALIC_KEY, font.italic());
    }

} // namespace EditorFont

} // namespace Fernanda::Ini
