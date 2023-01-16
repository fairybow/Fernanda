// toolbutton.cpp, Fernanda

#include "toolbutton.h"

ToolButton::ToolButton(Type type, QMainWindow* parentWindow)
    : type(type), parentWindow(parentWindow), QPushButton(parentWindow)
{
    setObjectName(QStringLiteral("toolButton"));
    installEventFilter(this);
    setCheckable(true);
    opacity->setOpacity(0.4);
    setGraphicsEffect(opacity);
    connect(this, &ToolButton::toggled, this, &ToolButton::action);
    connect(this, &ToolButton::toggled, this, [&](bool checked) { opacity->setEnabled(!opacity->isEnabled()); });
    typeDependentSetup();
}

void ToolButton::toggle(bool value)
{
    setVisible(value);
    Ud::saveConfig(configGroup, widgetConfig, value);
}

bool ToolButton::eventFilter(QObject* watched, QEvent* event)
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

void ToolButton::typeDependentSetup()
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
        connect(this, &ToolButton::startAwakeTimer, this, [&]() { stayAwakeTimer.value()->start(29000); });
        connect(stayAwakeTimer.value(), &QTimer::timeout, this, &ToolButton::stayAwake);
    }
    break;
    }
}

void ToolButton::alwaysOnTop(bool checked)
{
    if (checked)
        parentWindow->setWindowFlags(parentWindow->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
    else
        parentWindow->setWindowFlags(parentWindow->windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
    parentWindow->show();
}

void ToolButton::stayAwake()
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

void ToolButton::action(bool checked)
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

// toolbutton.cpp, Fernanda
