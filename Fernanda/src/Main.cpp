/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QStringList>

#include "Coco/StartCop.h"

#include "Application.h"
#include "BuildMessages.h"
#include "Version.h"

int main(int argc, char* argv[])
{
    Coco::StartCop cop(VERSION_APP_NAME_STRING, argc, argv);
    if (cop.isRunning()) return 0;

    Fernanda::Application app(argc, argv);

    app.connect(
        &cop,
        &Coco::StartCop::appRelaunched,
        &app,
        &Fernanda::Application::onRelaunchAttempted);

    app.initialize();
    return app.exec();
}
