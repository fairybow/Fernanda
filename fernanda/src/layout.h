// layout.h, Fernanda

#pragma once

#include <QStackedLayout>
#include <QVector>
#include <QWidget>

namespace Layout
{
    inline QWidget* stackWidgets(QVector<QWidget*> widgets, QWidget* parent = nullptr)
    {
        QWidget* container = new QWidget(parent);
        QStackedLayout* stack_layout = new QStackedLayout(container);
        stack_layout->setStackingMode(QStackedLayout::StackAll);
        for (auto& widget : widgets)
            stack_layout->addWidget(widget);
        return container;
    }

    inline QStackedLayout* stackLayout(QVector<QWidget*> widgets, QWidget* parent = nullptr)
    {
        QStackedLayout* stack_layout = new QStackedLayout(parent);
        stack_layout->setStackingMode(QStackedLayout::StackAll);
        for (auto& widget : widgets)
            stack_layout->addWidget(widget);
        return stack_layout;
    }
}

// layout.h, Fernanda
