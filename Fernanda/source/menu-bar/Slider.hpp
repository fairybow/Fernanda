#pragma once

#include "../common/Layout.hpp"
#include "../common/StringTools.hpp"
#include "../common/Widget.hpp"

#include <QLabel>
#include <QSlider>
#include <QString>
#include <QVector>

#include <optional>

class Slider : public Widget<>
{
	Q_OBJECT

public:
	Slider(const char* name, Qt::Orientation orientation, QWidget* parent = nullptr,
		const QString& label = QString(), bool hasDisplay = true, const QString& displayUnit = QString(), int multiplier = 1)
		: Widget(name, parent),
		m_displayUnit(displayUnit),
		m_multiplier(qBound(1, multiplier, 1000)),
		m_slider(new QSlider(orientation, this))
	{
		m_slider->setObjectName(name);
		setRange(0, 100);
		buildLabels(label, hasDisplay, displayUnit);
		buildLayout();
		connect(m_slider, &QSlider::valueChanged, this, [&](int value) {
			emit valueChanged(value * m_multiplier);
			});
	}

	static void setRange(QSlider* slider, int bottom, int top)
	{
		slider->setMinimum(bottom);
		slider->setMaximum(top);
	}

	void setRange(int bottom, int top)
	{
		setRange(m_slider, bottom, top);
	}

	void setValue(int value)
	{
		m_slider->setValue(value / m_multiplier);
	}

signals:
	void valueChanged(int value);

private:
	const QString m_displayUnit;
	const int m_multiplier;
	QSlider* m_slider;
	std::optional<QLabel*> m_label;
	std::optional<QLabel*> m_valueDisplay;

	void buildLabels(const QString& label, bool hasDisplay, const QString& displayUnit)
	{
		if (!label.isEmpty())
			m_label = new QLabel(label, this);

		if (hasDisplay) {
			m_valueDisplay = new QLabel(this);

			connect(m_slider, &QSlider::valueChanged, this, [&](int value) {
				auto final_value = value * m_multiplier;
				auto value_string = QString::number(final_value);
				auto text = m_displayUnit.isEmpty()
					? value_string
					: QString(value_string + " " + StringTools::sophisticatedPluralCheckThatAlwaysWorks(m_displayUnit, final_value));
				m_valueDisplay.value()->setText(text);
				});
		}
	}

	void buildLayout()
	{
		QWidgetList widgets;

		if (m_label.has_value())
			widgets.append(m_label.value());
		widgets << m_slider;
		if (m_valueDisplay.has_value())
			widgets.append(m_valueDisplay.value());

		auto layout = Layout::box(Layout::Line::Horizontally, widgets, this);
		Layout::setUniformSpacing(layout, 5);
	}
};
