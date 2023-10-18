#pragma once

#include "RegexPatterns.hpp"

#include <QRegularExpression>
#include <QString>
#include <QUrl>

#include <string>

inline QString operator%(const QString& lhs, const QString& rhs)
{
	return lhs + "<p>" + rhs;
}
inline QString operator%(const QString& lhs, const char* rhs) { return lhs % QString(rhs); }
inline QString operator%(const char* lhs, const QString& rhs) { return QString(lhs) % rhs; }
inline QString operator%(const QString& lhs, const std::string& rhs) { return lhs % QString::fromStdString(rhs); }
inline QString operator%(const std::string& lhs, const QString& rhs) { return QString::fromStdString(lhs) % rhs; }

inline QString operator%=(QString& lhs, const QString& rhs)
{
	lhs = lhs % rhs;
	return lhs;
}

inline QString operator/(const QString& lhs, const QString& rhs)
{
	return lhs + "<br>" + rhs;
}
inline QString operator/(const QString& lhs, const char* rhs) { return lhs / QString(rhs); }
inline QString operator/(const char* lhs, const QString& rhs) { return QString(lhs) / rhs; }
inline QString operator/(const QString& lhs, const std::string& rhs) { return lhs / QString::fromStdString(rhs); }
inline QString operator/(const std::string& lhs, const QString& rhs) { return QString::fromStdString(lhs) / rhs; }

namespace HtmlString
{
	namespace StdFs = std::filesystem;

	constexpr char TABLE_START[] = "<table><td>";
	constexpr char TABLE_DATA_START[] = "<td>";
	constexpr char TABLE_DATA_END[] = "</td>";
	constexpr char TABLE_END[] = "</table>";
	constexpr char EMPTY_TABLE_DATA[] = "<td>\n</td>";
	constexpr char FORMAT_BOLD[] = "<b>%1</b>";
	constexpr char FORMAT_HEADING[] = "<h%1>%2</h%1>";
	constexpr char FORMAT_LINK[] = "<a href='%1'>%2</a>";

	inline QString multiply(const char* character, int defaultArgument = 2)
	{
		if (defaultArgument < 1)
			defaultArgument = 1;
		QString multiplied;
		if (character[1] == '\0')
			multiplied = QString(defaultArgument, character[0]);
		else {
			for (auto i = 0; i < defaultArgument; ++i)
				multiplied += character;
		}
		return multiplied;
	}

	namespace
	{
		inline QString tableColumnSpacing(int columns = 9)
		{
			return multiply(EMPTY_TABLE_DATA, columns);
		}
	}

	template<typename T>
	inline QString table(const std::vector<T>& columns)
	{
		QString table = TABLE_START;
		for (auto& column : columns) {
			table += column + TABLE_DATA_END + tableColumnSpacing();
			table += (column != columns.back())
				? TABLE_DATA_START
				: TABLE_END;
		}
		return table;
	}

	template<typename T>
	inline QString bold(const T& text)
	{
		return QString(FORMAT_BOLD).arg(text);
	}

	template<typename T>
	inline QString heading(const T& text, int level = 1)
	{
		level = qBound(1, level, 6);
		return QString(FORMAT_HEADING).arg(level).arg(text);
	}

	inline QString link(const QString& url, QString displayName = QString())
	{
		if (displayName.isEmpty()) {
			QString url_copy = url;
			displayName = url_copy.replace(QRegularExpression(Regex::URL_BEGINNING), "");
		}
		return QString(FORMAT_LINK).arg(url).arg(displayName);
	}

	inline QString link(const QUrl& url, QString displayName = QString())
	{
		return link(url.toString(), displayName);
	}

	inline QString link(const char* url, QString displayName = QString())
	{
		return link(QString(url), displayName);
	}
}
