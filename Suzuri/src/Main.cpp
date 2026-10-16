/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include <QCoreApplication>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QtEnvironmentVariables>

#include <Coco/StartCop.h>

#include "App.h"
#include "core/Publication.h"

int main(int argc, char* argv[])
{
    auto exit_code = 0;
    auto restart = false;
    QString program{};
    QStringList arguments{};

    // The app and the single-instance guard live in this scope, so both are
    // gone before a restart starts the next copy. A copy started while the
    // guard is still listening would hand itself to this one and exit
    {
        Coco::StartCop cop(PUB_APP_NAME_STRING, argc, argv);

        if (cop.isRunning()) {
            return 0;
        }

        Suzuri::App app(argc, argv);
        app.connect(
            &cop,
            &Coco::StartCop::relaunched,
            &app,
            &Suzuri::App::onRelaunch);
        app.init();

        exit_code = app.exec();
        restart = app.restartRequested();

        // An AppImage runs from a folder that's unmounted when it exits, so
        // the next copy starts from the AppImage itself, which names itself in
        // APPIMAGE
        program = qEnvironmentVariableIsSet("APPIMAGE")
                      ? qEnvironmentVariable("APPIMAGE")
                      : QCoreApplication::applicationFilePath();
        arguments = QCoreApplication::arguments().mid(1);
    }

    if (restart) {
        QProcess::startDetached(program, arguments);
    }

    return exit_code;
}
