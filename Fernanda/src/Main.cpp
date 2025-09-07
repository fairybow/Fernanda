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

// Now/Next:
// - Tab dragging validator!
// - Ensure Workspace scoped menu toggles are updated on tab drag
// - Finish FileServiceSaveHelper::saveAsDialog_
// - Prepopulate file name for Save As dialog
// - File extensions for Save As
// - Ensure ViewService tracks views when tab is moved!
// - Is closing/saving "done"?

// (Now/Next) Notebook needs:
// - Open Notepad item will need to activate active window of already open
// Notepad else open new window
// - Get all menu actions down and note what Command they use (and other
// procedures if applicable) before implementing any of them
// - May need to fully rethink the close/save procedures (fill out Closing.md
// and Saving.md after menu changes)
// - A callback for closing tabs that saves the file to the temp file before
// closing the model. Still, we need to persist that there are changes so it can
// be marked dirty when reopened...
// - Need to address the problem of FileServiceSaveHelper::saveAsDialog_. May
// need a callback, to keep the module applicable to both Workspace types
// - Notebook Save As likely just means an enter-name dialog and append it to
// the end/top-level of the virtual structure
// - Both menus, New File (Save As with an empty txt)?
// - Rethink Root:
// - May want to change Workspace root member. Notebooks will take the archive
// path on construction, but their "root" for working documents will be
// TempExtractionDir/Content/. For Notepad, this root will be Documents/Fernanda
// (for now always, but later settable)
// - New Notebook file in Notepad menu (will create and then open)
// - Open Notebook file in Notepad menu (will filter specifically for .fnx)
// - Open Notepad in Notebook menu
// - Import/Export in Notebook menu
// - How do we handle saving as each window closes when we are closing the
// Notebook?
// - Determine how Model.xml will be verified on archive open each time
// - MenuModule -> NotepadMenuModule (with base MenuModule for both)?
// - Archive file watcher
// - File watcher for open temp dir files
// - Notebook archives will unpack to temp dir (with the temp dir saved as
// member). The files are named like UUID.txt. This information will be
// correlated with Model.xml, which provides the visual information users see in
// a Notebook TreeView

// Archive structure (i.e. MyProject.fnx):
// - Content (single-level working directory containing all files, named for
// their UUIDs)
// - Trash/Reference/Etc is conceptual. All is in content, but separated in view
// with Model.xml info
// - Model.xml (contains Contents dir display structure and parenting/orderings)
// - Settings.ini (Notebook's config override)

// Now/Next (But can wait till after Notebook):
// - WindowService: Keep a member of last size instead of using last window
// (since it doesn't register on app open)
// - WordCounter
// - Autosaving
// - How to update SettingsDialog if settings are changed via commander
// from outside?
// - Ensure ColorBar position is included in settings
// - On closing with a save prompt, when save is chosen, we may want a
// delay to show color bar and the tab unflagging as visual confirmation before
// closing (perhaps an API ColorBarModule can plug into to delay)
// - File watcher for open Notepad files
// - Key filters
// - Translations and translation change at run-time via menu

// Minor:
// - Check that we ever actually need Window::destroyed to emit pointer...
// - Check where we've relied on connecting to EventBus::windowDestroyed vs
// Window::destroyed...
// - Clean TreeViewModule::onWindowCreated_()
// - Could be the case that given our architecture, all services and modules
// don't really need to have any public methods (save ctor/dtor), as anything
// else is a query/command/call
// - Go through services and see what needs to be tracked and isn't (i.e. could
// improve discovery with enough significance to implement it), and what is
// tracked that doesn't need to be (i.e., may need to track file views, but
// don't necessarily need to track tree views)
// - Split files to .h/.cpp when "finished"
// - Un-generalize TabWidget
// - TabWidgetButton::paintEvent
// - Remove final period on class purpose statements if present
// - const check!
// - onFileReadied + updating on modification changed, use a singular "set
// tab properties" function?
// - I would maybe like to see the call site of queries/commands in log
// - Remove nullptr defaults for parentage where it is mandatory
// - Bug: So, the activate prev/next window menu shortcuts (and any menu
// shortcut) will stop working when the window is minimized. So, when cycling
// through windows, if one remains minimized (currently won't, since we make
// window->activate call showNormal if it's minimized), then the cycle can't
// continue
// - Query results not printing for window sets (likely just not able to
// print Qt containers)
// - A Fernanda::isDebug function. Its return should be defaulted to the
// build type but also toggleable at runtime
// - ViewService::onFileModificationChanged runs on every key stroke!
// Might be fine, might not be
// - Bug: plain text edit commands not logging (because QPTE has its own
// shortcuts that override our menu)
// - Pointer guard checks on all functions taking pointers (like
// file/window slots)
// - Change lists to sets where applicable
// - We get numbers for our enums in Command result print (need some
// way to make the toQString function just deduce whatever from QVariant)
// - Unified SavePrompt for one or more files (and just show checkboxes on
// > 1)
// - Ensure for closes we are always iterating backward (windows and
// views)
// - Ensure all pvt/"internal" functions/members have trailing underscore
// - Ensure all protected functions/members do NOT have trailing
// underscore
// - Reorg EventBus signal naming (like Commander)
// - Ensure all signals are logged in EventBus
// - Comments on declarations that we want viewable on Intellisense popup
// for things goes above them, otherwise add a space (like for a subheading)
// - Probably merge calls/commands?
// - "Scope" the version macros (prepending FERNANDA_)?
// - Check if we need active Window signal
// - Make WindowService::add private
// - In MenuMod toggles, note what each toggle's conditions are and when
// it should update
// - Go through call/query calls to make sure we're casting directly in
// the call/query template parameter (and not running to<T> on the result)
// - Wherever there is a window parameter for a command/call, we probably
// want to just use the context instead of the params map

// Code uniformity/clean-up:
// - Commander arg names may go in Constants
// - Find all quotes and make sure they're in TR if needed
// - Consistent order for args in event bus / slots (Window, View, Model,
// Index)
// - Clean includes (always include if used)
// - explicit on ctors that could have one arg (not multiple)
// - Fix all nested namespace to use `namespace Main::Sub {` syntax
// (instead of `namespace Main { namespace Sub {`)
// - Use delete (not deleteLater) everywhere to discover logic problems
// - Find unused functions
// - Rename MenuModule and Workspace to Abstract...

// Coco:
// - Rework/format Coco (again)!
// - Rename Coco file save dialog to save as, also consider making the
// path change a coco return
// - qMemoryAddress use QObjectPointer concept instead of QObjectDerived?

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
