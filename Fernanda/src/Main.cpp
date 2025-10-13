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
/// - ColorBarModule commands, remove workspaceOpened and also save signals,
/// probably
/// - Note this in Bus.md: Window service command handlers should be responsible
/// for showing. Later, when we open multiple windows, we will have a different
/// command, perhaps (or different args for the same command), letting the
/// command handler still handle showing windows
/// - Implement menu commands
/// - Document utility commands (like windowsReversed, etc)
/// - Redo Bus signals (events)
/// - Comment-out Utility and see what's needed
/// - Reimplement Commands
/// - Consider if Command is Notebook- or Notepad-specific, like NewTab and
/// NewTreeViewModel: need to be same command (with different implementation),
/// CANNOT have separate namespaces, because users of those commands don't know
/// or care about workspace type
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
/// - Log to file (commented-out method is too slow)
///
/// Clean-up:
///
/// - Search TODO
/// - Remove "NOTE:" before notes, use only TODO or it's a note
/// - Clean includes (Commands were in Constants briefly)
/// - Check license statements
/// - For Bus, ensure we use the windowDestroyed signal and don't connect to
/// window destroyed signals inside our onWindowCreated functions...
/// - Uniform use of nodiscard
///
/// Command handler registering sites:
/// - Make sure we aren't casting return values to QVar when registering (it
/// isn't needed!)
/// - Can we just return Coco::Path without QString conversion?
/// - Check where we can remove Utility.h include (toQVariant unneeded)
/// - Also check lambda args

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
