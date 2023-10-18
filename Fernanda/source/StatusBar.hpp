#pragma once

#include "common/Widget.hpp"

#include <QLabel>
#include <QStatusBar>
#include <QVector>

class StatusBar : public Widget<QStatusBar>
{
public:
	StatusBar(const char* name, QWidget* parent = nullptr)
		: Widget(name, parent)
	{
		setMaximumHeight(22);
	}

	void addWidgets(QWidgetList leftWidgets, QWidgetList rightWidgets)
	{
		for (auto& widget : leftWidgets)
			if (widget != nullptr)
				addPermanentWidget(widget, 0);

		addPermanentWidget(m_spacer, 1);

		for (auto& widget : rightWidgets)
			if (widget != nullptr)
				addPermanentWidget(widget, 0);
	}

	void addWidgets(QWidget* leftWidget, QWidget* rightWidget)
	{
		addWidgets(QWidgetList{ leftWidget }, QWidgetList{ rightWidget });
	}

	void addLeftWidget(QWidget* leftWidget)
	{
		addWidgets(QWidgetList{ leftWidget }, QWidgetList{});
	}

	void addRightWidget(QWidget* rightWidget)
	{
		addWidgets(QWidgetList{}, QWidgetList{ rightWidget });
	}

private:
	QLabel* m_spacer = new QLabel;
};
