// toolbutton.h, Fernanda

#pragma once

#include "userdata.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPushButton>

class ToolButton : public QPushButton
{
    Q_OBJECT

public:
    ToolButton(QWidget* parent = nullptr)
    {
        installEventFilter(this);
        setCheckable(true);
        opacity->setOpacity(0.4);
        setGraphicsEffect(opacity);
        connect(this, &ToolButton::toggled, this, [&](bool checked)
            {
                opacity->setEnabled(!opacity->isEnabled());
            });
    }

    void toggle(Ud::ConfigVal valueType, bool value)
    {
        setVisible(value);
        Ud::saveConfig(Ud::ConfigGroup::Window, valueType, value);
    }

private:
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);

    bool eventFilter(QObject* watched, QEvent* event)
    {
        ToolButton* button = qobject_cast<ToolButton*>(watched);
        if (!button) return false;
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
        {
            opacity->setEnabled(!opacity->isEnabled());
            return true;
        }
        return false;
    }
};

// toolbutton.h, Fernanda
