// tool.cpp, Fernanda

#include "tool.h"

Tool::Tool(Type type, QMainWindow* parentWindow)
    : type(type), parentWindow(parentWindow), QPushButton(parentWindow)
{
    setObjectName(QStringLiteral("tool"));
    installEventFilter(this);
    setCheckable(true);
    opacity->setOpacity(0.4);
    setGraphicsEffect(opacity);
    connect(this, &Tool::toggled, this, [&](bool checked) { opacity->setEnabled(!opacity->isEnabled()); });
    typeDependentSetup();
}

void Tool::toggle(bool value)
{
    if (isChecked())
        setChecked(false);
    setVisible(value);
    UserData::saveConfig(configGroup, widgetConfig, value);
}

void Tool::setTimerValue(int value)
{
    time = value;
    UserData::saveConfig(configGroup, UserData::IniValue::ToolTimer, value);
}

bool Tool::eventFilter(QObject* watched, QEvent* event)
{
    Tool* button = qobject_cast<Tool*>(watched);
    if (!button) return false;
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
    {
        opacity->setEnabled(!opacity->isEnabled());
        return true;
    }
    return false;
}

void Tool::typeDependentSetup()
{
    switch (type) {
    case Type::AlwaysOnTop:
    {
        setText(Icon::draw(Icon::Name::Pushpin));
        widgetConfig = UserData::IniValue::ToggleToolAOT;
        actionConfig = UserData::IniValue::AlwaysOnTop;
        connect(this, &Tool::toggled, this, [&](bool checked)
            {
                alwaysOnTop();
                UserData::saveConfig(configGroup, actionConfig.value(), checked);
            });
    }
    break;
    case Type::StayAwake:
    {
        setText(Icon::draw(Icon::Name::Tea));
        widgetConfig = UserData::IniValue::ToggleToolSA;
        actionConfig = UserData::IniValue::StayAwake;
        connect(this, &Tool::toggled, this, [&](bool checked)
            {
                stayAwake();
                UserData::saveConfig(configGroup, actionConfig.value(), checked);
            });
        timer = new QTimer(this);
        auto& timer_value = timer.value();
        timer_value->setTimerType(Qt::VeryCoarseTimer);
        connect(this, &Tool::startTimer, this, [&]() { timer_value->start(29000); });
        connect(timer_value, &QTimer::timeout, this, [&]() { stayAwake(); });
    }
    break;
    case Type::Timer:
    {
        setText(Icon::draw(Icon::Name::Timer));
        widgetConfig = UserData::IniValue::ToggleToolTimer;
        timer = new QTimer(this);
        auto& timer_value = timer.value();
        timer_value->setTimerType(Qt::PreciseTimer);
        timer_value->setSingleShot(true);
        connect(this, &Tool::toggled, this, [&](bool checked)
            {
                (checked) ? timer_value->start(time.value()) : timer_value->stop();
            });
        connect(timer.value(), &QTimer::timeout, this, [&]()
            {
                Popup::timeUp();
                setChecked(false);
            });
    }
    break;
    }
}

void Tool::alwaysOnTop()
{
    if (isChecked())
        parentWindow->setWindowFlags(parentWindow->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
    else
        parentWindow->setWindowFlags(parentWindow->windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
    parentWindow->show();
}

void Tool::stayAwake()
{

#ifdef Q_OS_WINDOWS

    if (!isChecked())
        SetThreadExecutionState(ES_CONTINUOUS);
    else
    {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
        startTimer();
    }

#endif

}

// tool.cpp, Fernanda
