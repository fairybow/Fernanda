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

/// Current goals:
/// - Reimplement Commands
/// - Consider if Command is Notebook- or Notepad-specific
/// - Take NewTab and NewTreeViewModel: need to be same command (with different
/// implementation), CANNOT have separate namespaces, because users of those
/// commands don't know or care about workspace type
/// - Clean-up: ensure all command handler registrations use "param" instead of
/// "to"
/// - Plan handling of NewTab, FileSave, FileClose, and similar
/// - Plan Application Quit
/// - Reimplement Notepad menu
/// - Build Notebook menu
/// - Ensure we pass -1 as index param for edit commands
/// - Implement Notebook TreeView model
/// - Implement model.xml
/// - Implement .fnx

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
