#include "Stylist.h"

Stylist::Stylist(QWidgetList widgets, QObject* parent,
	const QString& baseSuffix, const QString& extension)
	: QObject(parent), m_baseSuffix(baseSuffix), m_extension(extension)
{
	for (auto& widget : widgets)
		add(widget);
}

void Stylist::add(QWidget* widget)
{
	auto [base_path, style_path] = sheetPathsFromClass(widget);
	m_stylees << Stylee{ widget, base_path, style_path };
}

void Stylist::style(QWidget* styleeWidget, const StdFsPath& themeSheet)
{
	setCurrentThemeSheet(styleeWidget, themeSheet);
	styleAll();
}

void Stylist::style()
{
	styleAll();
}

void Stylist::unStyle()
{
	styleAll(Mode::Unstyle);
}

void Stylist::setThemeEnabled(QWidget* styleeWidget, bool hasTheme)
{
	apply([styleeWidget, hasTheme](Stylee& stylee) {
		if (stylee.widget == styleeWidget)
			stylee.hasTheme = hasTheme;
		});
	styleAll();
}

Stylist::StdPathPair Stylist::sheetPathsFromClass(QWidget* widget)
{
	QString class_name = widget->metaObject()->className();
	auto root = Path::toStdFs(":/stylist");
	auto base_file_name = class_name + m_baseSuffix + m_extension;
	auto style_file_name = class_name + m_extension;
	auto base_path = root / base_file_name.toStdString();
	auto style_path = root / style_file_name.toStdString();
	return { base_path, style_path };
}

void Stylist::apply(std::function<void(Stylee&)> function)
{
	for (auto i = 0; i < m_stylees.count(); ++i)
		function(m_stylees[i]);
}

void Stylist::styleAll(Mode mode)
{
	apply([&](Stylee& stylee) {
		(mode == Mode::Style)
			? stylee.widget->setStyleSheet(buildFullStyleSheet(stylee))
			: stylee.widget->setStyleSheet(nullptr);
		});
}

void Stylist::setCurrentThemeSheet(QWidget* styleeWidget, const StdFsPath& themeSheetPath)
{
	apply([styleeWidget, themeSheetPath](Stylee& stylee) {
		if (stylee.widget == styleeWidget)
			stylee.currentThemeSheet = themeSheetPath;
		});
}

QString Stylist::buildFullStyleSheet(const Stylee& stylee)
{
	auto full_style_sheet = Io::readFile(stylee.baseSheet);
	if (stylee.hasTheme && stylee.currentThemeSheet != StdFsPath()) {
		auto theme_sheet = Io::readFile(stylee.currentThemeSheet);
		full_style_sheet += "\n"
			+ buildThemedStyleSheet(Io::readFile(stylee.styleSheet), theme_sheet);
	}
	return full_style_sheet;
}

const QString Stylist::buildThemedStyleSheet(QString styleSheet, const QString& themeSheet)
{
	static const auto theme_sheet_line = QRegularExpression(StyleRegex::THEME_SHEET_LINE);
	auto matches = theme_sheet_line.globalMatch(themeSheet);
	while (matches.hasNext()) {
		auto match = matches.next();
		if (!match.hasMatch()) continue;
		auto [variable, value] = variableAndValue(match);
		styleSheet.replace(QRegularExpression(variable), value);
	}
	return styleSheet;
}

Stylist::QStringPair Stylist::variableAndValue(const QRegularExpressionMatch& themeSheetMatch)
{
	static const auto theme_sheet_value = QRegularExpression(StyleRegex::THEME_SHEET_VALUE);
	static const auto theme_sheet_variable = QRegularExpression(StyleRegex::THEME_SHEET_VARIABLE);
	QString variable = themeSheetMatch.captured(0).replace(theme_sheet_value, nullptr);
	QString value = themeSheetMatch.captured(0).replace(theme_sheet_variable, nullptr);
	return { variable, value };
}
