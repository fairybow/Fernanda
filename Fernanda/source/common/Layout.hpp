#pragma once

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMargins>
#include <QStackedLayout>
#include <QString>
#include <QVBoxLayout>
#include <QVector>

namespace Layout
{
	enum class Line { Horizontally, Vertically };
	enum class Type { Box, Stack };

	namespace
	{
		inline void add(QLayout* layout, QWidgetList widgets)
		{
			for (auto& widget : widgets) {
				if (widget == nullptr) continue;
				layout->addWidget(widget);
			}
		}

		inline void setBoxProperties(QBoxLayout* box, QWidgetList widgets, QMargins margins)
		{
			box->setContentsMargins(margins);
			box->setSpacing(0);
			add(box, widgets);
		}

		inline void setStackProperties(QStackedLayout* stack, QWidgetList widgets)
		{
			stack->setContentsMargins(QMargins());
			stack->setSpacing(0);
			stack->setStackingMode(QStackedLayout::StackAll);
			add(stack, widgets);
		}

		inline void setGridProperties(QGridLayout* grid, QWidgetList widgets, QMargins margins)
		{
			grid->setContentsMargins(margins);
			grid->setSpacing(0);
			add(grid, widgets);
		}
	}

	inline QBoxLayout* box(Line alignment, QWidgetList widgets = {}, QWidget* parent = nullptr, QMargins margins = QMargins())
	{
		QBoxLayout* layout = nullptr;
		(alignment == Line::Horizontally)
			? layout = new QHBoxLayout(parent)
			: layout = new QVBoxLayout(parent);

		setBoxProperties(layout, widgets, margins);

		if (parent)
			parent->setLayout(layout);

		return layout;
	}

	inline QBoxLayout* box(Line alignment, QWidget* widget, QWidget* parent = nullptr, QMargins margins = QMargins())
	{
		return box(alignment, QWidgetList{ widget }, parent, margins);
	}

	inline QStackedLayout* stack(QWidgetList widgets, QWidget* parent = nullptr)
	{
		auto layout = new QStackedLayout(parent);
		setStackProperties(layout, widgets);
		if (parent != nullptr)
			parent->setLayout(layout);
		return layout;
	}

	inline QStackedLayout* stack(QWidget* widget, QWidget* parent = nullptr)
	{
		return stack(QWidgetList{ widget }, parent);
	}

	inline QGridLayout* grid(QWidgetList widgets, QWidget* parent = nullptr, QMargins margins = QMargins())
	{
		auto layout = new QGridLayout(parent);
		setGridProperties(layout, widgets, margins);
		if (parent != nullptr)
			parent->setLayout(layout);
		return layout;
	}

	inline QGridLayout* grid(QWidget* widget, QWidget* parent = nullptr, QMargins margins = QMargins())
	{
		return grid(QWidgetList{ widget }, parent, margins);
	}

	inline void transpareForMouse(QWidgetList widgets)
	{
		for (auto& widget : widgets) {
			if (widget == nullptr) continue;
			widget->setAttribute(Qt::WA_TransparentForMouseEvents);
		}
	}

	inline void transpareForMouse(QWidget* widget)
	{
		transpareForMouse({ widget });
	}

	inline void setUniformSpacing(QVector<QLayout*> layouts, int spacing = 5)
	{
		for (auto& layout : layouts) {
			layout->setContentsMargins(spacing, spacing, spacing, spacing);
			layout->setSpacing(spacing);
		}
	}

	inline void setUniformSpacing(QLayout* layout, int spacing = 5)
	{
		setUniformSpacing(QVector<QLayout*>{ layout }, spacing);
	}

	inline QWidget* container(QLayout* layout, QWidget* parent = nullptr)
	{
		auto container = new QWidget(parent);
		container->setLayout(layout);
		return container;
	}

	inline QWidget* labeledContainer(QWidgetList widgets, QWidget* parent = nullptr, const QString& label = QString(), Line alignment = Line::Vertically)
	{
		auto container = new QWidget(parent);
		auto layout = box(alignment, widgets, container);
		if (label.isEmpty()) return container;
		auto q_label = new QLabel(label);
		layout->insertWidget(0, q_label);
		setUniformSpacing(layout);
		return container;
	}

	inline QWidget* labeledContainer(QWidget* widget, QWidget* parent = nullptr, const QString& label = QString(), Line alignment = Line::Vertically)
	{
		return labeledContainer(QWidgetList{ widget }, parent, label, alignment);
	}

	inline QWidget* labeledContainer(QLayout* layout, QWidget* parent = nullptr, const QString& label = QString(), Line alignment = Line::Vertically)
	{
		return labeledContainer(container(layout, parent), parent, label, alignment);
	}
}
