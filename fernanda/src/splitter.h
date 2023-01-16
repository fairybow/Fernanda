// splitter.h, Fernanda

#pragma once

#include "userdata.h"

#include <QByteArray>
#include <QSplitter>
#include <QVector>
#include <QWidget>

class Splitter : public QSplitter
{
    Q_OBJECT

public:
    Splitter(QWidget* parent = nullptr)
    {
        setObjectName("splitter");
        connect(this, &QSplitter::splitterMoved, this, [&]()
            {
                Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Splitter, saveState());
            });
    }

    void addWidgets(QVector<QWidget*> widgets)
    {
        for (auto& widget : widgets)
            addWidget(widget);
        setCollapsible(0, true);
        auto editor = 1;
        setCollapsible(editor, false);
        setStretchFactor(editor, 100);
    }

    void loadConfig(QRect geometry)
    {
        auto state = Ud::loadConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Splitter, QVariant()).toByteArray();
        if (state.isEmpty() || state.isNull())
            setSizes(QVector<int>{ static_cast<int>(geometry.width() * 0.2), static_cast<int>(geometry.width() * 0.8) });
        else
            restoreState(state);
    }
};

// splitter.h, Fernanda
