// text.cpp, Fernanda

#include "text.h"

const QString Text::operator%(QString lhs, const char* rhs)
{
	return lhs + QStringLiteral("<p>") + rhs;
}

const QString Text::operator%(QString lhs, QString rhs)
{
	return lhs + QStringLiteral("<p>") + rhs;
}

const QString Text::operator/(QString lhs, const char* rhs)
{
	return lhs + QStringLiteral("<br>") + rhs;
}

const QString Text::operator/(QString lhs, QString rhs)
{
	return lhs + QStringLiteral("<br>") + rhs;
}

const QRegularExpression Text::regex(Re operation)
{
	QRegularExpression result;
	switch (operation) {
	case Re::Forbidden:
		result = QRegularExpression(QStringLiteral(R"((<|>|:|\/|\\|\||\?|\*|\"))"));
		break;
	case Re::NewLine:
		result = QRegularExpression(QStringLiteral("(\\n)"));
		break;
	case Re::ParagraphSeparator:
		result = QRegularExpression(QStringLiteral("(\U00002029)"));
		break;
	case Re::Space:
		result = QRegularExpression(QStringLiteral("(\\s)"));
		break;
	case Re::Split:
		result = QRegularExpression(QStringLiteral("(\\s|\\n|\\r|\U00002029|^)+"));
		break;
	case Re::ThemeSheetCursor:
		result = QRegularExpression(QStringLiteral("(@cursorColor; = )(.*)(;)"));
		break;
	case Re::ThemeSheetCursorUnder:
		result = QRegularExpression(QStringLiteral("(@cursorUnderColor; = )(.*)(;)"));
		break;
	case Re::ThemeSheetLine:
		result = QRegularExpression(QStringLiteral("(@.*\\n?)"));
		break;
	case Re::ThemeSheetValue:
		result = QRegularExpression(QStringLiteral("(\\s=.*;)"));
		break;
	case Re::ThemeSheetVariable:
		result = QRegularExpression(QStringLiteral("(@.*=\\s)"));
		break;
	case Re::UrlBeginning:
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

const QString Text::multiSpaces(int spaces)
{
	return multiplyThese(" ", spaces);
}

const QString Text::newLines(int lines)
{
	return multiplyThese("\n", lines);
}

const QString Text::tableColumnSpacing(int columns)
{
	return multiplyThese("<td>\n</td>", columns);
}

const QString Text::heading(const char* text)
{
	return QStringLiteral("<h3><b>") + text + QStringLiteral("</b></h3>");
}

const QString Text::bold(const char* text)
{
	return QStringLiteral("<b>") + text + QStringLiteral("</b>");
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
		displayName = QString(url).replace(regex(Re::UrlBeginning), nullptr);
	return QStringLiteral("<a href='") + url + QStringLiteral("'>") + displayName + QStringLiteral("</a>");
}

const QString Text::change(bool isQuit)
{
	QString result;
	auto base = QStringLiteral("You have ") + bold("unsaved changes") + QStringLiteral(". Are you sure you want to ");
	(isQuit)
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

const QString Text::openUdButton()
{
	return pad("Open the user data folder");
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

const QString Text::shortcuts()
{
	return heading("Shortcuts") % table({ menuShortcuts(), windowEditorShortcuts() });
}

const QString Text::repo()
{
	return link("https://github.com/fairybow/fernanda");
}

const QString Text::releases()
{
	return link("https://github.com/fairybow/fernanda/releases");
}

const QString Text::ghApi()
{
	return QStringLiteral("https://api.github.com/repos/fairybow/fernanda/releases");
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

const QString Text::timeUp()
{
	return QStringLiteral("Time's up!") + multiSpaces(10);
}

// text.cpp, Fernanda
