#pragma once

#include "RegexPatterns.hpp"

#include <QRegularExpression>
#include <QString>
#include <QVector>

#include <ctime>
#include <utility>

namespace StringTools
{
	enum class Side { Both, Left, Right };

	namespace
	{
		using QStringPair = std::pair<QString, QString>;

		const QVector<QStringPair> doubleEscapeReplacements = {
		{"\\\\", "\\"},
		{"\\'", "\'"},
		{"\\\"", "\""},
		{"\\?", "\?"},
		{"\\a", "\a"},
		{"\\b", "\b"},
		{"\\f", "\f"},
		{"\\n", "\n"},
		{"\\r", "\r"},
		{"\\t", "\t"},
		{"\\v", "\v"}
		};

		bool isEven(int x)
		{
			return x % 2 == 0;
		}
	}

	inline QString time()
	{
		auto now = std::time(0);
		auto time = std::ctime(&now);
		return QString::fromLocal8Bit(time).replace("  ", " ").trimmed();
	}

	inline QString fixEscapes(const QString& string)
	{
		QString cleaned_string = string;
		for (auto& [from, to] : doubleEscapeReplacements)
			cleaned_string.replace(from, to);
		return cleaned_string;
	}

	inline QString removeForbidden(const QString& string)
	{
		QString cleaned_string = string;
		return cleaned_string.replace(QRegularExpression(Regex::FORBIDDEN), "_");
	}

	inline QString nonAlphaNumericToSpaces(const QString& string)
	{
		QString cleaned_string = string;
		for (auto i = 0; i < cleaned_string.size(); ++i)
			if (!cleaned_string[i].isLetterOrNumber())
				cleaned_string[i] = ' ';
		return cleaned_string;
	}

	inline QString capitalize(const QString& string)
	{
		QString capitalized_string = string;
		auto word_start = true;

		for (auto i = 0; i < capitalized_string.size(); ++i) {
			if (word_start && capitalized_string[i].isLetter()) {
				capitalized_string[i] = capitalized_string[i].toUpper();
				word_start = false;
			}
			else if (capitalized_string[i].isSpace())
				word_start = true;
		}

		return capitalized_string;
	}

	inline const QString secondsToMinutes(int seconds, const char* separator = ":")
	{
		auto time_seconds = seconds % 60;
		QString seconds_string;
		(time_seconds <= 9)
			? seconds_string = "0" + QString::number(time_seconds)
			: seconds_string = QString::number(time_seconds);
		return QString::number((seconds / 60) % 60) + separator + seconds_string;
	}

	inline QString sophisticatedPluralCheckThatAlwaysWorks(const QString& text, int value)
	{
		if (!text.endsWith("s") || value != 1) return text;
		return text.left(text.length() - 1);
	}

	inline QString flank(const QString& string, int desiredLength, Side side = Side::Right,
		QChar flankChar = ' ', bool separate = false)
	{
		auto length = string.length();
		if (length >= desiredLength)
			return string;

		auto times = desiredLength - length;
		if (side == Side::Both) {
			if (!isEven(times))
				--times;
			times /= 2;
		}

		auto flanking = QString(flankChar).repeated(times);

		QString separator;
		if (separate) {
			separator = ' ';
			flanking = flanking.mid(0, flanking.length() - 1);
		}

		QString flanked;
		switch (side) {
		case Side::Both:
			flanked = flanking + separator + string + separator + flanking;
			break;
		case Side::Left:
			flanked = flanking + separator + string;
			break;
		case Side::Right:
			flanked = string + separator + flanking;
			break;
		}
		return flanked;
	}

	template<typename... Strings>
	inline QString padAll(int spaces, const Strings&... string)
	{
		auto padding = QString(" ").repeated(spaces);
		QStringList list{ string... };
		return padding + list.join(padding) + padding;
	}

	inline QString pad(int spaces, const QString& string, Side side = Side::Both)
	{
		auto padding = QString(" ").repeated(spaces);
		QString padded;
		switch (side) {
		case Side::Both:
			padded = padding + string + padding;
			break;
		case Side::Left:
			padded = padding + string;
			break;
		case Side::Right:
			padded = string + padding;
			break;
		}
		return padded;
	}
}
