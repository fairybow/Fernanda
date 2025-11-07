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
/// - (DONE) Opening files via TreeView in both Workspaces
/// -- (DONE) So, hook up TreeView base class signals for clicking first (could
/// emit bus event)
/// -- (DONE) Could be picked up by Workspace subclasses instead of FileService
/// this time? (signal emits QModelIndex)
/// -- (DONE) Then Notepad/Notebook needs to get path (OS/UUID+ext in xml)?
/// -- Separate SaveServices for each Workspace?
/// - (DONE) NewTab behavior for both Workspaces
/// -- (DONE) Should FileService have a separate command for off-disk files?
/// - (DONE) Opening files via Menu in Notepad (Notebook menu won't open, just import)
/// - (DONE) Notebook Import
/// - Moving/reorganizing Notebook files in TreeView
/// - Marking Notebook as modified
///
/// Opening Files doc will cover:
/// - Use same FileService (make Opening Files/Saving Files docs)
/// - Talk about the different ways each workspace uses FileService
/// - Opening files via TreeView and Menu for both Workspaces
/// - NewTab behavior for both Workspaces
///
/// Saving Files doc will cover:
/// - Notebook auto save will only have to be when last view on model closes
/// (tab closes if it's last tab for that file) but can still be periodic on
/// timer too
/// - It's also possible we may want to just keep all models open in Notebook
/// even after last view on file closes? Probably not, though
/// - Notepad saves
/// - Notepad auto save (methods, dir)
/// - Notebook save
/// - Notebook auto save
/// - Notebook internal auto save, change persistence
///
/// Also Next:
/// - UserData doc
/// - Replace Coco/TextIo with project version
/// - Implementing menus
/// - Redoing commands for menus as it is reimplemented
/// - Utility commands (windowsReversed, etc)
/// - Document in Menus.md and Bus.md
/// - Redo/organize Bus events
/// - Implement menu toggles
///
/// - Could rethink Services/Modules distinction to frame Services as proactive
/// and modules as more reactive, rather than the current
/// essential/non-essential distinction (seeing a pattern where, for example,
/// FileService is fairly proactive and ViewService is primarily reactive, with
/// slots making up the majority of its work)
/// -- It would be purely nominal based on function, as each would still inherit
/// IService
///
/// Things to log:
/// - Active window
/// - Active file view/model
/// - Debug or Utility function that shows a popup for the message (but isn't
/// fatal)
/// -- Could also run red color bar for window
///
/// Coco:
/// - Redo path, potentially reintegrate PathUtil with Path
/// -- Or, better, Io umbrella file (dialogs in there with read/writes?)
/// - Figure out Path string caching
/// - Ensure Path's shared data works
/// - isFolder to isDir
/// - Path::copy (or Coco::copy)?
/// - Move mkdir top level (Coco::mkdir) or Path?
/// - Move other non-Path stuff there (search TODO here)
/// - I know it's not necessary and is how std::fs::path works, but Path without
/// separator normalization sucks
///
/// Clean-up:
/// - QDomDocument::ParseResult good model for what we need for maybe save
/// result or similar
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
/// - Also check lambda args (whether it uses a Command or not)

/// Unsorted (but next at some point):
/// - QSet<T> printing, notably for WINDOWS_SET command result. Won't be able to
/// with current approach (template type). Need something to store type info
/// alongside command handler maybe and then handle all printing in Bus
/// -- Could maybe add a callback for debug printer (returns QString) that can
/// be added while registering handler and will correctly print the result
/// (wouldn't need one for params, since we'd likely never pass list, set, or
/// other template type)
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
