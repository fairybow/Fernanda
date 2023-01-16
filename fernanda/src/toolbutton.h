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

    ToolButton(Type type, QMainWindow* parentWindow = nullptr);
    void toggle(bool value);

private:
    Type type{};
    Ud::ConfigGroup configGroup = Ud::ConfigGroup::Window;
    Ud::ConfigVal widgetConfig;
    Ud::ConfigVal actionConfig;
    QMainWindow* parentWindow;
    std::optional<QTimer*> stayAwakeTimer;
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);

    bool eventFilter(QObject* watched, QEvent* event);
    void typeDependentSetup();
    void alwaysOnTop(bool checked);
    void stayAwake();

private slots:
    void action(bool checked);

signals:
    void startAwakeTimer();
};

// toolbutton.h, Fernanda
