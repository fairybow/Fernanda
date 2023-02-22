/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Tool.h, Fernanda

#pragma once

#include "Icon.h"
#include "Popup.h"
#include "StatusBarButton.h"
#include "UserData.h"

#include <QMainWindow>
#include <QMouseEvent>
#include <Qt>
#include <QTimer>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <optional>

class Tool : public StatusBarButton
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
    
    void typeDependentSetup();
    void alwaysOnTop();
    void stayAwake();
    const QString time(int seconds);

private slots:
    void startCountdown(bool checked);
    void countdownDisplay();

signals:
    int resetCountdown();
    void startAwakeTimer();
};

// Tool.h, Fernanda
