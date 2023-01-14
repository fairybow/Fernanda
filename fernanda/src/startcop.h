// startcop.h, Fernanda

#pragma once

/*#include <qsystemdetection.h>

#ifdef Q_OS_WINDOWS

#include <windows.h>
#include <winuser.h>

#endif*/

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <Qt>
#include <QString>
#include <QWidget>

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

#ifdef Q_OS_WINDOWS

    void focusMainWindow()
    {
        for (auto& widget : QApplication::allWidgets())
        {
            if (widget->objectName() != "mainWindow") continue;
            if (widget->windowState() == Qt::WindowMinimized)
                widget->setWindowState((widget->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            //auto name = widget->windowTitle().toStdWString();
            //auto handle = FindWindow(0, name.c_str());
            //SwitchToThisWindow(handle, FALSE); // [This function is not intended for general use. It may be altered or unavailable in subsequent versions of Windows.]
            widget->activateWindow();
            break;
        }
    }

#elif Q_OS_LINUX

    void focusMainWindow()
    {
        for (auto& widget : QApplication::allWidgets())
        {
            if (widget->objectName() != "mainWindow") continue;
            if (widget->windowState() == Qt::WindowMinimized)
                widget->setWindowState((widget->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            widget->activateWindow();
            break;
        }
    }

#endif

};

// startcop.h, Fernanda
