/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Text.cpp, Fernanda

#include "Text.h"

const QRegularExpression Text::regex(Regex operation)
{
	QRegularExpression result;
	switch (operation) {
	case Regex::Forbidden:
		result = QRegularExpression(QStringLiteral(R"((<|>|:|\/|\\|\||\?|\*|\"))"));
		break;
	case Regex::NewLine:
		result = QRegularExpression(QStringLiteral("(\\n)"));
		break;
	case Regex::ParagraphSeparator:
		result = QRegularExpression(QStringLiteral("(\U00002029)"));
		break;
	case Regex::Space:
		result = QRegularExpression(QStringLiteral("(\\s)"));
		break;
	case Regex::Split:
		result = QRegularExpression(QStringLiteral("(\\s|\\n|\\r|\U00002029|^)+"));
		break;
	case Regex::ThemeSheetCursor:
		result = QRegularExpression(QStringLiteral("(@cursorColor; = )(.*)(;)"));
		break;
	case Regex::ThemeSheetCursorUnder:
		result = QRegularExpression(QStringLiteral("(@cursorUnderColor; = )(.*)(;)"));
		break;
	case Regex::ThemeSheetLine:
		result = QRegularExpression(QStringLiteral("(@.*\\n?)"));
		break;
	case Regex::ThemeSheetValue:
		result = QRegularExpression(QStringLiteral("(\\s=.*;)"));
		break;
	case Regex::ThemeSheetVariable:
		result = QRegularExpression(QStringLiteral("(@.*=\\s)"));
		break;
	case Regex::UrlBeginning:
		result = QRegularExpression(QStringLiteral("(https:\\/\\/|www.)"));
		break;
	}
	return result;
}

const QString Text::multiplyThese(QString character, int defaultArgument)
{
	if (defaultArgument < 1)
		defaultArgument = 1;
	QString result;
	for (auto i = 0; i < defaultArgument; ++i)
		result.append(character);
	return result;
}

const QString Text::pad(const char* text, int spaces)
{
	QString padding = multiSpaces(spaces);
	return padding + text + padding;
}

const QString Text::pad(QString text, int spaces)
{
	QString padding = multiSpaces(spaces);
	return padding + text + padding;
}

const QString Text::table(QStringList columns)
{
	QString result = QStringLiteral("<table><td>");
	for (auto& column : columns)
	{
		result.append(column).append(QStringLiteral("</td>") + tableColumnSpacing());
		(column != columns.last())
			? result.append(QStringLiteral("<td>"))
			: result.append(QStringLiteral("</table>"));
	}
	return result;
}

const QString Text::link(const char* url, QString displayName)
{
	if (displayName.isEmpty())
		displayName = QString(url).replace(regex(Regex::UrlBeginning), nullptr);
	return QStringLiteral("<a href='") + url + QStringLiteral("'>") + displayName + QStringLiteral("</a>");
}

const QString Text::change(bool isQuit)
{
	QString result;
	auto base = QStringLiteral("You have ") + bold("unsaved changes") + QStringLiteral(". Are you sure you want to ");
	isQuit
		? result = base + QStringLiteral("quit?")
		: result = base + QStringLiteral("change stories?");
	return result;
}

const QString Text::saveAndButtons(bool isQuit)
{
	if (isQuit)
		return pad("Save and quit");
	return pad("Save and change");
}

const QString Text::samples()
{
	return
	{
		QStringLiteral("A sample font, window theme, and editor theme have been added to your user data folder.") %
		QStringLiteral("You'll need to ") + bold("restart") + QStringLiteral(" to see custom themes and fonts incorporated.")
	};
}

const QString Text::menuShortcuts()
{
	return
	{
		bold("Menu:") %
		QStringLiteral("Ctrl + S: Save story") /
		QStringLiteral("Ctrl + Q: Quit")
	};
}

const QString Text::windowEditorShortcuts()
{
	return
	{
		bold("Window/Editor:") %
		QStringLiteral("F11: Cycle editor themes (Amber, Green, Grey)") /
		QStringLiteral("Alt + F10: Cycle fonts") /
		QStringLiteral("Alt + F11: Cycle editor themes (all)") /
		QStringLiteral("Alt + F12: Cycle window themes") /
		QStringLiteral("Alt + Insert: Nav previous") /
		QStringLiteral("Alt + Delete: Nav next") /
		QStringLiteral("Alt + Minus (-) /") /
		QStringLiteral("Ctrl + Mouse Wheel Down: Decrease font size") /
		QStringLiteral("Alt + Plus (+) /") /
		QStringLiteral("Ctrl + Mouse Wheel Up: Increase font size") /
		QStringLiteral("Ctrl + Y: Redo") /
		QStringLiteral("Ctrl + Z: Undo") /
		QStringLiteral("Ctrl + Shift + C: Wrap selection or block in quotes")
	};
}

const QString Text::about()
{
	return
	{
		heading("About Fernanda") %
		QStringLiteral("Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)") %
		QStringLiteral("It's a personal project and a work-in-progress.") %
		heading("Version") %
		VER_FILEVERSION_STR %
		QStringLiteral("See ") + repo() + QStringLiteral(" for more information.")
	};
}

const QString Text::version(VersionCheck check, QString latestVersion)
{
	QString base =
	{
		heading("Version") %
		bold("Current version:") /
		VER_FILEVERSION_STR
	};
	QString message;
	switch (check) {
	case VersionCheck::Error:
		message =
		{
			QStringLiteral("Unable to verify version.") %
			bold("Check:") /
			releases()
		};
		break;
	case VersionCheck::Latest:
		message = QStringLiteral("You have the latest version.");
		break;
	case VersionCheck::Old:
		message =
		{
			bold("New version:") /
			latestVersion %
			QStringLiteral("You do not have the latest version.") %
			bold("Download:") /
			releases()
		};
		break;
	}
	return
	{
		base %
		message
	};
}

const QString Text::totalCounts(int lines, int words, int characters)
{
	return
	{
		heading("Total Counts") %
		table({
			QStringLiteral("Lines:") /
			QStringLiteral("Words:") /
			QStringLiteral("Characters:"),
			QString::number(lines) /
			QString::number(words) /
			QString::number(characters)
			})
	};
}

// Text.cpp, Fernanda
