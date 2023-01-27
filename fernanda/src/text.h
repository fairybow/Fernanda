// text.h, Fernanda

#pragma once

#include "version.h"

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

	const QString operator%(QString lhs, const char* rhs);
	const QString operator%(QString lhs, QString rhs);
	const QString operator/(QString lhs, const char* rhs);
	const QString operator/(QString lhs, QString rhs);
	const QRegularExpression regex(Regex operation);
	const QString multiplyThese(QString character, int defaultArgument = 1);
	const QString multiSpaces(int spaces = 3);
	const QString newLines(int lines = 2);
	const QString tableColumnSpacing(int columns = 9);
	const QString heading(const char* text);
	const QString bold(const char* text);
	const QString pad(const char* text, int spaces = 3);
	const QString pad(QString text, int spaces = 3);
	const QString table(QStringList columns);
	const QString link(const char* url, QString displayName = nullptr);
	const QString change(bool isQuit = false);
	const QString saveAndButtons(bool isQuit = false);
	const QString openUdButton();
	const QString samples();
	const QString menuShortcuts();
	const QString windowEditorShortcuts();
	const QString shortcuts();
	const QString repo();
	const QString releases();
	const QString ghApi();
	const QString about();
	const QString version(VersionCheck check, QString latestVersion);
	const QString timeUp();
	const QString totalCounts(int lines, int words, int characters);
}

// text.h, Fernanda
