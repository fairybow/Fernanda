// toolbutton.h, Fernanda

#pragma once

#include "icon.h"
#include "userdata.h"

#include <optional>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

class ToolButton : public QPushButton
{
    Q_OBJECT

public:
    enum class Type {
        AlwaysOnTop,
        StayAwake
    };

    ToolButton(Type type, QMainWindow* parent = nullptr)
        : type(type), window(parent), QPushButton(parent)
    {
        installEventFilter(this);
        setCheckable(true);
        opacity->setOpacity(0.4);
        setGraphicsEffect(opacity);
        connect(this, &ToolButton::toggled, this, &ToolButton::action);
        connect(this, &ToolButton::toggled, this, [&](bool checked) { opacity->setEnabled(!opacity->isEnabled()); });
        typeDependentSetup();
    }

    void toggle(bool value)
    {
        setVisible(value);
        Ud::saveConfig(configGroup, widgetConfig, value);
    }

private:
    Type type{};
    Ud::ConfigGroup configGroup = Ud::ConfigGroup::Window;
    Ud::ConfigVal widgetConfig;
    Ud::ConfigVal actionConfig;
    QMainWindow* window;
    std::optional<QTimer*> stayAwakeTimer;
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

    void typeDependentSetup()
    {
        switch (type) {
        case Type::AlwaysOnTop:
        {
            setText(Icon::draw(Icon::Name::Pushpin));
            widgetConfig = Ud::ConfigVal::T_AotBtn;
            actionConfig = Ud::ConfigVal::Aot;
        }
        break;
        case Type::StayAwake:
        {
            setText(Icon::draw(Icon::Name::Tea));
            widgetConfig = Ud::ConfigVal::T_AwakeBtn;
            actionConfig = Ud::ConfigVal::Awake;
            stayAwakeTimer = new QTimer(this);
            connect(this, &ToolButton::startAwakeTimer, this, [&]() { stayAwakeTimer.value()->start(15000); });
            connect(stayAwakeTimer.value(), &QTimer::timeout, this, &ToolButton::stayAwake);
        }
        break;
        }
    }

    void alwaysOnTop(bool checked)
    {
        if (checked)
            window->setWindowFlags(window->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
        else
            window->setWindowFlags(window->windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
        window->show();
    }

    void stayAwake()
    {

#ifdef Q_OS_WINDOWS

        if (!isChecked())
            SetThreadExecutionState(ES_CONTINUOUS);
        else
        {
            SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
            startAwakeTimer();
        }

#endif

    }

private slots:
    void action(bool checked)
    {
        switch (type) {
        case Type::AlwaysOnTop:
            alwaysOnTop(checked);
            break;
        case Type::StayAwake:
            startAwakeTimer();
            break;
        }
        Ud::saveConfig(configGroup, actionConfig, checked);
    }

signals:
    void startAwakeTimer();
};

// toolbutton.h, Fernanda
