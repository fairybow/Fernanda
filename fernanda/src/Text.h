/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Text.h, Fernanda

#pragma once

#include "Version.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace Text
{
	enum class Regex {
		Forbidden,
		NewLine,
		ParagraphSeparator,
		Space,
		Split,
		ThemeSheetCursor,
		ThemeSheetCursorUnder,
		ThemeSheetLine,
		ThemeSheetValue,
		ThemeSheetVariable,
		UrlBeginning
	};
	enum class VersionCheck {
		Error,
		Latest,
		Old
	};

	const QRegularExpression regex(Regex operation);
	const QString multiplyThese(QString character, int defaultArgument = 1);
	const QString pad(const char* text, int spaces = 3);
	const QString pad(QString text, int spaces = 3);
	const QString table(QStringList columns);
	const QString link(const char* url, QString displayName = nullptr);
	const QString change(bool isQuit = false);
	const QString saveAndButtons(bool isQuit = false);
	const QString samples();
	const QString menuShortcuts();
	const QString windowEditorShortcuts();
	const QString about();
	const QString version(VersionCheck check, QString latestVersion);
	const QString totalCounts(int lines, int words, int characters);

	inline const QString operator%(QString lhs, const char* rhs) { return lhs + QStringLiteral("<p>") + rhs; }
	inline const QString operator%(QString lhs, QString rhs) { return lhs + QStringLiteral("<p>") + rhs; }
	inline const QString operator/(QString lhs, const char* rhs) { return lhs + QStringLiteral("<br>") + rhs; }
	inline const QString operator/(QString lhs, QString rhs) { return lhs + QStringLiteral("<br>") + rhs; }
	inline const QString bold(const char* text) { return QStringLiteral("<b>") + text + QStringLiteral("</b>"); }
	inline const QString gitHubApi() { return QStringLiteral("https://api.github.com/repos/fairybow/fernanda/releases"); }
	inline const QString heading(const char* text) { return QStringLiteral("<h3><b>") + text + QStringLiteral("</b></h3>"); }
	inline const QString multiSpaces(int spaces = 3) { return multiplyThese(" ", spaces); }
	inline const QString newLines(int lines = 2) { return multiplyThese("\n", lines); }
	inline const QString openUdButton() { return pad("Open the user data folder"); }
	inline const QString releases() { return link("https://github.com/fairybow/fernanda/releases"); }
	inline const QString repo() { return link("https://github.com/fairybow/fernanda"); }
	inline const QString shortcuts() { return heading("Shortcuts") % table({ menuShortcuts(), windowEditorShortcuts() }); }
	inline const QString tableColumnSpacing(int columns = 9) { return multiplyThese("<td>\n</td>", columns); }
	inline const QString timeUp() { return QStringLiteral("Time's up!") + multiSpaces(10); }
}

// Text.h, Fernanda
