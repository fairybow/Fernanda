#pragma once

#include <utility>

#include <QFont>
#include <QTextOption>
#include <QtGlobal>

#include "Settings.h"

namespace Ini
{
    namespace Groups
    {
        constexpr auto EDITOR = "Editor";
        constexpr auto EDITOR_FONT = "EditorFont";
    }

    namespace Editor
    {
        constexpr auto COS_KEY = "CenterOnScroll";
        constexpr auto DEFAULT_COS = false;

        constexpr auto OVERWRITE_KEY = "Overwrite";
        constexpr auto DEFAULT_OVERWRITE = false;

        constexpr auto TAB_STOP_PX_MIN = 20;
        constexpr auto TAB_STOP_PX_MAX = 160;
        constexpr auto TAB_STOP_PX_KEY = "TabStopPixels";
        constexpr auto DEFAULT_TAB_STOP_PX = 80;

        constexpr auto WORD_WRAP_MODE_KEY = "WordWrapMode";
        constexpr auto DEFAULT_WORD_WRAP_MODE = QTextOption::WrapAtWordBoundaryOrAnywhere;

        struct Values
        {
            // Use defaults above? Or should we somehow indicate (if
            // unfortunately subtly) that the load failed by doing all false?
            bool centerOnScroll = false;
            bool overwrite = false;
            int tabStopPx = 0;
            QTextOption::WrapMode wordWrapMode{};
        };

        inline Values load(Settings* settings)
        {
            Values values{};
            if (!settings) return values;
            settings->beginGroup(Groups::EDITOR);

            values.centerOnScroll = settings->value<bool>(COS_KEY, DEFAULT_COS);
            values.overwrite = settings->value<bool>(OVERWRITE_KEY, DEFAULT_OVERWRITE);
            values.tabStopPx = settings->value<int>(TAB_STOP_PX_KEY, DEFAULT_TAB_STOP_PX);
            values.wordWrapMode = settings->value<QTextOption::WrapMode>(WORD_WRAP_MODE_KEY, DEFAULT_WORD_WRAP_MODE);

            settings->endGroup();
            return values;
        }

        inline void save(const Values& values, Settings* settings)
        {
            if (!settings) return;

            settings->beginGroup(Groups::EDITOR);
            settings->setValue(COS_KEY, values.centerOnScroll);
            settings->setValue(OVERWRITE_KEY, values.overwrite);
            settings->setValue(TAB_STOP_PX_KEY, values.tabStopPx);
            settings->setValue(WORD_WRAP_MODE_KEY, values.wordWrapMode);
            settings->endGroup();
        }
    }

    namespace EditorFont
    {
        constexpr auto FAMILY_KEY = "Family";
        constexpr auto DEFAULT_FAMILY = "mononoki";

        constexpr auto PT_SIZE_KEY = "Size";
        constexpr auto PT_SIZE_MIN = 8;
        constexpr auto PT_SIZE_MAX = 72;
        constexpr auto DEFAULT_PT_SIZE = 14;

        constexpr auto BOLD_KEY = "Bold";
        constexpr auto DEFAULT_BOLD = false;

        constexpr auto ITALIC_KEY = "Italic";
        constexpr auto DEFAULT_ITALIC = false;

        inline QFont load(Settings* settings)
        {
            QFont font{};
            if (!settings) return font;
            settings->beginGroup(Groups::EDITOR_FONT);

            font.setFamily(settings->value<QString>(FAMILY_KEY, DEFAULT_FAMILY));

            auto font_size = settings->value<int>(PT_SIZE_KEY, DEFAULT_PT_SIZE);
            font.setPointSize(qBound(PT_SIZE_MIN, font_size, PT_SIZE_MAX));

            font.setBold(settings->value<bool>(BOLD_KEY, DEFAULT_BOLD));
            font.setItalic(settings->value<bool>(ITALIC_KEY, DEFAULT_ITALIC));

            settings->endGroup();
            return font;
        }

        inline void save(const QFont& font, Settings* settings)
        {
            if (!settings) return;

            settings->beginGroup(Groups::EDITOR_FONT);
            settings->setValue(FAMILY_KEY, font.family());
            settings->setValue(PT_SIZE_KEY, font.pointSize());
            settings->setValue(BOLD_KEY, font.bold());
            settings->setValue(ITALIC_KEY, font.italic());
            settings->endGroup();
        }
    }
}
