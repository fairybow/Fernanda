// text.h, Fernanda

#pragma once

#include "version.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace Text
{
	enum class Re {
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
	enum class Version {
		Error,
		Latest,
		Old
	};

	const QString operator%(QString lhs, const char* rhs);
	const QString operator%(QString lhs, QString rhs);
	const QString operator/(QString lhs, const char* rhs);
	const QString operator/(QString lhs, QString rhs);
	const QRegularExpression regex(Re operation);
	const QString multiplyThese(QString character, int defaultArg = 1);
	const QString spaces(int spaces = 3);
	const QString newLines(int lines = 2);
	const QString heading(const char* text);
	const QString bold(const char* text);
	const QString pad(const char* text);
	const QString table(QStringList columns);
	const QString link(const char* url, QString displayName = nullptr);
	const QString change(bool isQuit = false);
	const QString saveAndButtons(bool isQuit = false);
	const QString openUdButton();
	const QString samples();
	const QString menuShortcuts();
	const QString windowShortcuts();
	const QString editorShortcuts();
	const QString shortcuts();
	const QString repo();
	const QString releases();
	const QString about();
	const QString version(Version check, QString latestVersion);
}

// text.h, Fernanda
