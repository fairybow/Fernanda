// style.h, Fernanda

#pragma once

#include "io.h"
#include "text.h"

#include <QAction>
#include <QActionGroup>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

namespace Style
{
    namespace StdFs = std::filesystem;

    enum class WinStyle {
        BaseOnly,
        WithTheme
    };

    struct EditorGroup {
        QString styleSheet = nullptr;
        QString cursorColor = nullptr;
        QString underCursorColor = nullptr;
    };

    inline const QString createStyleSheetFromTheme(QString styleSheet, QString themeSheet)
    {
        QRegularExpressionMatchIterator matches = Text::regex(Text::Re::ThemeSheetLine).globalMatch(themeSheet);
        while (matches.hasNext())
        {
            QRegularExpressionMatch match = matches.next();
            if (!match.hasMatch()) continue;
            QString variable = match.captured(0).replace(Text::regex(Text::Re::ThemeSheetValue), nullptr);
            QString value = match.captured(0).replace(Text::regex(Text::Re::ThemeSheetVariable), nullptr);
            styleSheet.replace(QRegularExpression(variable), value);
        }
        return styleSheet;
    }

    inline const EditorGroup editorStyle(StdFs::path themePath, bool hasTheme, bool hasShadow)
    {
        auto style_sheet = Io::readFile(":/themes/editor_base.qss");
        QString cursor_color = nullptr;
        QString under_cursor_color = nullptr;
        if (hasTheme)
        {
            auto theme_sheet = Io::readFile(themePath);
            style_sheet = style_sheet + Text::newLines() + createStyleSheetFromTheme(Io::readFile(":/themes/editor.qss"), theme_sheet);
            QRegularExpressionMatch match_cursor = Text::regex(Text::Re::ThemeSheetCursor).match(theme_sheet);
            QRegularExpressionMatch match_under_cursor = Text::regex(Text::Re::ThemeSheetCursorUnder).match(theme_sheet);
            cursor_color = match_cursor.captured(2);
            under_cursor_color = match_under_cursor.captured(2);
        }
        if (hasShadow)
            style_sheet = style_sheet + Text::newLines() + Io::readFile(":/themes/shadow.qss");
        return EditorGroup{ style_sheet, cursor_color, under_cursor_color };
    }

    inline const QString windowStyle(StdFs::path themePath, bool hasTheme, WinStyle baseOnly = WinStyle::WithTheme)
    {
        auto style_sheet = Io::readFile(":/themes/window_base.qss");
        if (hasTheme && baseOnly != WinStyle::BaseOnly)
        {
            auto theme_sheet = Io::readFile(themePath);
            style_sheet = style_sheet + Text::newLines() + createStyleSheetFromTheme(Io::readFile(":/themes/window.qss"), theme_sheet);
        }
        return style_sheet;
    }

    inline void actionCycle(QActionGroup* group)
    {
        auto actions = group->actions();
        auto current_theme = group->checkedAction();
        if (current_theme != actions.last())
        {
            auto set_next = false;
            for (auto& action : actions)
            {
                if (set_next)
                {
                    action->setChecked(true);
                    break;
                }
                if (action == current_theme)
                    set_next = true;
            }
        }
        else
            actions.first()->setChecked(true);
    }
}

// style.h, Fernanda
