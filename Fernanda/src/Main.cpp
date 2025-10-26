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

/// Next:
/// - FNX doc and UserData doc
/// - Replace Coco/TextIo with project version
/// - Tree view stuff
/// - New Notebook first! Figure out fnx creation
/// - As a consequence, also figure out what members Notebook needs
/// (archive/save dir, temp dir, anything else?)
/// - FnxModel class for TreeView?
/// - Implementing menus
/// - Redoing commands for menus as it is reimplemented
/// - Utility commands (windowsReversed, etc)
/// - Document in Menus.md and Bus.md
/// - Redo/organize Bus events
/// - Implement menu toggles
/// - Deciding if OS and Archive FSs need different services
///
/// Things to log:
/// - Active window
/// - Active file view/model
///
/// Coco:
/// - Redo path, potentially reintegrate PathUtil with Path
/// - Move stuff there (search TODO here)
/// - Path::copy (or Coco::copy)?
/// - Move mkdir top level (Coco::mkdir)?
///
/// Clean-up:
/// - Comment-out Utility and see what's needed
/// - Ensure all command handler registrations use "param" instead of
/// "to"
/// - Search TODO
/// - Remove "NOTE:" before notes, use only TODO or it's a note
/// - For Bus, ensure we use the windowDestroyed signal and don't connect to
/// window destroyed signals inside our onWindowCreated functions...
/// - Uniform use of nodiscard
/// - Check lambda args (whether it needs & or = or should explicitly capture)
///
/// Command handler registering sites clean-up:
/// - Make sure we aren't casting return values to QVar when registering (it
/// isn't needed!)
/// - Can we just return Coco::Path without QString conversion?
/// - Check where we can remove Utility.h include (toQVariant unneeded)
/// - Also check lambda args (whether it uses a Command or not)

/// Unsorted (but next at some point):
/// - Ability to log Coco::Bool
/// - Consider if Command is Notebook- or Notepad-specific, like NewTab and
/// NewTreeViewModel: need to be same command (with different implementation),
/// CANNOT have separate namespaces, because users of those commands don't know
/// or care about workspace type
/// - Plan handling of NewTab, FileSave, FileClose, and similar
/// - Plan Application Quit
/// - Implement Notebook TreeView model
/// - Implement model.xml
/// - Implement .fnx
/// - Log to file (commented-out method is too slow)
///
/// Unsorted clean-up:
/// - Standardize callback code for close acceptor and similar
/// - Find code that needs to be sectioned-off into a function for clarity
/// (check slots and setup functions, for example)
/// - Split to h/cpp
/// - Move Internal namespaces to source
/// - Ensure Internals are _ postfixed and possibly without the namespace, once
/// in source
/// - Clean includes (Commands were in Constants briefly)
/// - Check license statements

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
