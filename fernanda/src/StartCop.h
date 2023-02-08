/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// StartCop.h, Fernanda

#pragma once

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <Qt>
#include <QString>
//#include <qsystemdetection.h>
#include <QWidget>

/*#ifdef Q_OS_WINDOWS

#include <Windows.h>
#include <WinUser.h>

#endif*/

class StartCop : public QObject
{
    Q_OBJECT

public:
    StartCop(const QString& name)
        : name(name) {}

    bool exists()
    {
        if (serverExists()) return true;
        startServer();
        return false;
    }

private:
    const QString name;

    bool serverExists()
    {
        QLocalSocket socket;
        socket.connectToServer(name);
        bool exists = socket.isOpen();
        socket.close();
        return exists;
    }

    void startServer()
    {
        QLocalServer* server = new QLocalServer;
        server->setSocketOptions(QLocalServer::WorldAccessOption);
        server->listen(name);
        connect(server, &QLocalServer::newConnection, this, &StartCop::focusMainWindow);
    }

private slots:

    void focusMainWindow()
    {
        for (auto& widget : QApplication::allWidgets())
        {
            if (widget->objectName() != "mainWindow") continue;
            if (widget->windowState() == Qt::WindowMinimized)
                widget->setWindowState((widget->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

/*#ifdef Q_OS_WINDOWS

            auto name = widget->windowTitle().toStdWString();
            auto handle = FindWindow(0, name.c_str());
            SwitchToThisWindow(handle, FALSE);
            // [This function is not intended for general use. It may be altered or unavailable in subsequent versions of Windows.]

#endif*/

            widget->activateWindow();
            break;
        }
    }
};

// StartCop.h, Fernanda
