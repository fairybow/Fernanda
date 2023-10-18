#pragma once

#include "common/Io.hpp"
#include "common/Path.hpp"
#include "common/StylePatterns.hpp"

#include <QRegularExpressionMatchIterator>
#include <QWidget>
#include <QVector>

#include <functional>
#include <utility>

class Stylist : public QObject
{
public:
	using StdFsPath = std::filesystem::path;
	using StdPathPair = std::pair<StdFsPath, StdFsPath>;
	using QStringPair = std::pair<QString, QString>;

	Stylist(QWidgetList widgets = {}, QObject* parent = nullptr,
		const QString& baseSuffix = "Base", const QString& extension = ".qss");

	void add(QWidget* widget);
	void style(QWidget* styleeWidget, const StdFsPath& themeSheet);
	void style();
	void unStyle();
	void setThemeEnabled(QWidget* styleeWidget, bool hasTheme);

	void devClass()
	{
		qDebug() << __FUNCTION__;

		for (auto& stylee : m_stylees) {
			qDebug() << "Widget:"
				<< stylee.widget;
			qDebug() << "Base sheet path:"
				<< Path::toQString(stylee.baseSheet);
			qDebug() << "Style sheet path:"
				<< Path::toQString(stylee.styleSheet);
			qDebug() << "Current theme sheet path:"
				<< Path::toQString(stylee.currentThemeSheet);
			qDebug() << "Has theme?:" << stylee.hasTheme
				<< Qt::endl;
		}
	}

	void devStyleSheets()
	{
		qDebug() << __FUNCTION__;

		for (auto& stylee : m_stylees) {
			qDebug() << "Widget:"
				<< stylee.widget;
			qDebug() << stylee.widget->styleSheet()
				<< Qt::endl;
		}
	}

private:
	enum class Mode { Style, Unstyle };

	struct Stylee {
		QWidget* widget;
		StdFsPath baseSheet;
		StdFsPath styleSheet;
		StdFsPath currentThemeSheet = StdFsPath();
		bool hasTheme = true;
	};

	const QString& m_baseSuffix;
	const QString& m_extension;
	QVector<Stylee> m_stylees;

	StdPathPair sheetPathsFromClass(QWidget* widget);
	void apply(std::function<void(Stylee&)> function);
	void styleAll(Mode mode = Mode::Style);
	void setCurrentThemeSheet(QWidget* styleeWidget, const StdFsPath& themeSheetPath);
	QString buildFullStyleSheet(const Stylee& stylee);
	const QString buildThemedStyleSheet(QString styleSheet, const QString& themeSheet);
	QStringPair variableAndValue(const QRegularExpressionMatch& themeSheetMatch);
};
