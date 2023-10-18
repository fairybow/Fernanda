/*#pragma once

#include "Xml.hpp"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QSize>
#include <QString>
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QTextStream>
#include <QPainter>
#include <QPixmap>

#include <map>

namespace Svg
{
	enum class Ui {
		None,
		Add,
		ChevronBack,
		ChevronDown,
		ChevronForward,
		ChevronUp,
		Close,
		Ellipse
	};

	namespace
	{
		const std::map<Ui, QString> uiPaths = {
		{ Ui::Add, ":/ui/add-outline.svg" },
		{ Ui::ChevronBack, ":/ui/chevron-back-outline.svg" },
		{ Ui::ChevronDown, ":/ui/chevron-down-outline.svg" },
		{ Ui::ChevronForward, ":/ui/chevron-forward-outline.svg" },
		{ Ui::ChevronUp, ":/ui/chevron-up-outline.svg" },
		{ Ui::Close, ":/ui/close-outline.svg" },
		{ Ui::Ellipse, ":/ui/ellipse.svg" }
		};

		QByteArray adjustColor(const QString& fileName, const QColor& color)
		{
			QFile file(fileName);
			QDomDocument document;
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text)
				|| !document.setContent(&file))
				return QByteArray();

			auto color_name = color.name();
			Xml::replaceAttributeCssValue(document,
				"line", "style", "fill", color_name);
			Xml::replaceAttributeCssValue(document,
				"line", "style", "stroke", color_name);
			Xml::replaceAttributeCssValue(document,
				"polyline", "style", "stroke", color_name);
			Xml::addTagAttribute(document,
				"path", "fill", color_name);
			Xml::addTagAttribute(document,
				"path", "fill", color_name);

			QBuffer buffer;
			buffer.open(QIODevice::WriteOnly);
			QTextStream stream(&buffer);
			document.save(stream, 0);
			return buffer.data();
		}

		inline QIcon icon(const QString& fileName, const QColor& color = Qt::black, double scale = 1.0)
		{
			auto data = adjustColor(fileName, color);
			QSvgRenderer renderer(data);
			QPixmap pixmap(QSize(18, 18) *= scale);
			pixmap.fill(Qt::transparent);
			QPainter painter(&pixmap);
			renderer.render(&painter);
			return QIcon(pixmap);
		}
	}

	inline QIcon ui(Ui name, const QColor& color = Qt::black, double scale = 1.0)
	{
		auto it = uiPaths.find(name);
		if (it != uiPaths.end())
			return icon(it->second, color, scale);
		return QIcon();
	}
}
*/