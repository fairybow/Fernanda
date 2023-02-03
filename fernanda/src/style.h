/*  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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

    struct EditorGroup {
        QString styleSheet = nullptr;
        QString cursorColor = nullptr;
        QString underCursorColor = nullptr;
    };

    inline const QString createStyleSheetFromTheme(QString styleSheet, QString themeSheet)
    {
        QRegularExpressionMatchIterator matches = Text::regex(Text::Regex::ThemeSheetLine).globalMatch(themeSheet);
        while (matches.hasNext())
        {
            QRegularExpressionMatch match = matches.next();
            if (!match.hasMatch()) continue;
            QString variable = match.captured(0).replace(Text::regex(Text::Regex::ThemeSheetValue), nullptr);
            QString value = match.captured(0).replace(Text::regex(Text::Regex::ThemeSheetVariable), nullptr);
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
            QRegularExpressionMatch match_cursor = Text::regex(Text::Regex::ThemeSheetCursor).match(theme_sheet);
            QRegularExpressionMatch match_under_cursor = Text::regex(Text::Regex::ThemeSheetCursorUnder).match(theme_sheet);
            cursor_color = match_cursor.captured(2);
            under_cursor_color = match_under_cursor.captured(2);
        }
        if (hasShadow)
            style_sheet = style_sheet + Text::newLines() + Io::readFile(":/themes/shadow.qss");
        return EditorGroup{ style_sheet, cursor_color, under_cursor_color };
    }

    inline const QString windowStyle(StdFs::path themePath, bool hasTheme)
    {
        auto style_sheet = Io::readFile(":/themes/window_base.qss");
        if (hasTheme)
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

    inline void dump(QActionGroup* group, StdFs::path path)
    {
        auto dump_directory = path / "__dump";
        Path::makeDirs(dump_directory);
        for (auto& action : group->actions())
        {
            auto data = action->data();
            Path::copy(Path::toStdFs(data), dump_directory / Path::getName<StdFs::path>(data, true));
        }
    }
}

// style.h, Fernanda
