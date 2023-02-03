/*  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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

void Tool::setCountdown(int seconds)
{
    countdown = seconds;
    UserData::saveConfig(configGroup, UserData::IniValue::ToolTimer, seconds);
}

void Tool::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        setChecked(!isChecked());
        return;
    }
    if (type == Type::Timer)
    {
        if (countdown > 0 && countdown < resetCountdown())
        {
            auto& timer_value = timer.value();
            timer_value->isActive()
                ? timer_value->stop()
                : timer_value->start(1000);
            return;
        }
    }
    QPushButton::mousePressEvent(event);
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
        connect(this, &Tool::startAwakeTimer, this, [&]() { timer_value->start(29000); });
        connect(timer_value, &QTimer::timeout, this, [&]() { stayAwake(); });
    }
    break;
    case Type::Timer:
    {
        setText(Icon::draw(Icon::Name::Timer));
        widgetConfig = UserData::IniValue::ToggleToolTimer;
        connect(this, &Tool::toggled, this, &Tool::startCountdown);
    }
    break;
    }
}

void Tool::alwaysOnTop()
{
    isChecked()
        ? parentWindow->setWindowFlags(parentWindow->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
        : parentWindow->setWindowFlags(parentWindow->windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
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
        startAwakeTimer();
    }

#endif

}

const QString Tool::time(int seconds)
{
    auto time_seconds = seconds % 60;
    QString seconds_string;
    (time_seconds <= 9)
        ? seconds_string = "0" + QString::number(time_seconds)
        : seconds_string = QString::number(time_seconds);
    return QString::number((seconds / 60) % 60) + "." + seconds_string;
}

void Tool::startCountdown(bool checked)
{
    if (!timer.has_value())
    {
        timer = new QTimer(this);
        auto& timer_value = timer.value();
        timer_value->setTimerType(Qt::PreciseTimer);
        connect(timer_value, &QTimer::timeout, this, &Tool::countdownDisplay);
    }
    auto& timer_value = timer.value();
    if (!checked)
    {
        setText(Icon::draw(Icon::Name::Timer));
        timer_value->stop();
        countdown = resetCountdown();
        return;
    }
    timer_value->start(1000);
}

void Tool::countdownDisplay()
{
    auto& countdown_value = countdown.value();
    setText(Text::pad(Icon::draw(Icon::Name::Timer) + " " + time(countdown_value), 2));
    if (countdown_value < 1)
    {
        Popup::timeUp();
        setChecked(false);
        return;
    }
    --countdown_value;
}

// tool.cpp, Fernanda
