// tool.h, Fernanda

#pragma once

#include "icon.h"
#include "popup.h"
#include "userdata.h"

#include <optional>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <Qt>
#include <QTimer>

class Tool : public QPushButton
{
    Q_OBJECT

public:
    enum class Type {
        AlwaysOnTop,
        StayAwake,
        Timer
    };

    Tool(Type type, QMainWindow* parentWindow = nullptr);
    void toggle(bool value);

public slots:
    void setCountdown(int seconds);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    Type type{};
    UserData::IniGroup configGroup = UserData::IniGroup::Window;
    UserData::IniValue widgetConfig;
    std::optional<UserData::IniValue> actionConfig;
    std::optional<QTimer*> timer;
    std::optional<int> countdown;
    QMainWindow* parentWindow;
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);

    bool eventFilter(QObject* watched, QEvent* event);
    void typeDependentSetup();
    void alwaysOnTop();
    void stayAwake();
    const QString time(int seconds);

private slots:
    void startCountdown(bool checked);
    void countdownDisplay();

signals:
    void startAwakeTimer();
    int resetCountdown();
};

// tool.h, Fernanda
