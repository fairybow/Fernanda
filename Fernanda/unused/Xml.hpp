/*#pragma once

#include "RegexPatterns.hpp"

#include <QDomDocument>
#include <QRegularExpression>
#include <QString>

namespace Xml
{
	inline void replaceAttributeCssValue(QDomDocument document, const QString& tag,
		const QString& attribute, const QString& css, const QString& value)
	{
		auto nodes = document.elementsByTagName(tag);
		for (auto i = 0; i < nodes.count(); ++i) {
			auto element = nodes.at(i).toElement();
			auto style = element.attribute(attribute);
			auto regex = QRegularExpression(
				QString(Regex::FORMAT_XML_ATTRIBUTE_CSSVALUE).arg(css));
			style.replace(regex, QString("%1:").arg(css) + value);
			element.setAttribute(attribute, style);
		}
	}

	inline void addAttributeCssValue(QDomDocument document, const QString& tag,
		const QString& attribute, const QString& css, const QString& value)
	{
		auto nodes = document.elementsByTagName(tag);
		for (auto i = 0; i < nodes.count(); ++i) {
			auto element = nodes.at(i).toElement();
			auto style = element.attribute(attribute);
			style += QString(";%1:%2").arg(css).arg(value);
			element.setAttribute(attribute, style);
		}
	}

	inline void addTagAttribute(QDomDocument document, const QString& tag,
		const QString& attribute, const QString& value)
	{
		auto nodes = document.elementsByTagName(tag);
		for (auto i = 0; i < nodes.count(); ++i) {
			auto element = nodes.at(i).toElement();
			element.setAttribute(attribute, value);
		}
	}
}
*/