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
// - Possibly base tree view then two subclasses with OS file model and custom
// archive model
// - StatusBar module (or just managed by window service) (may emit status bar
// created to alert WordCounterModule, TreeViewModule (for toggle button), etc
// - Utility status bar method (if we subclass status bar, to add spacer in
// middle and addleft/addright)

// (Now/Next) Notebook needs:
// - Plan archive structure
// - MenuModule -> NotepadMenuModule (with base MenuModule for both)?
// - How to handle Notebook paths? VPath wrapper class? We'll need
// something that for Notepad works as a normal path but for Notebooks allows us
// to, when needed (which is most of the time, save its Notepad settings
// fallback path), redirect relative archive paths to OS temp folders, i.e.
// `notebook1-root/file.txt` becomes, under the hood,
// `os-temp-dir/notebook1/file.txt`
// - An Open dialog onto the archive (good god how)
// - Tree view service/module
// - Archive file watcher

// Now/Next (But can wait till after Notebook):
// - WordCounter
// - Autosaving
// - How to update SettingsDialog if settings are changed via commander
// from outside?
// - Ensure ColorBar position is included in settings
// - On closing with a save prompt, when save is chosen, we may want a
// delay to show color bar and the tab unflagging as visual confirmation before
// closing (perhaps an API ColorBarModule can plug into to delay)
// - File watcher for Notepad

// Minor:
// - Clean TreeViewModule::initialize_()
// - Clean includes (always include if used)
// - Could be the case that given our architecture, all services and modules
// don't really need to have any public methods (save ctor/dtor), as anything
// else is a query/command/call
// - Go through services and see what needs to be tracked and isn't (i.e. could
// improve discovery with enough significance to implement it), and what is
// tracked that doesn't need to be (i.e., may need to track file views, but
// don't necessarily need to track tree views)
// - Split files to .h/.cpp when "finished"
// - explicit on ctors that could have one arg (not multiple)
// - Fix all nested namespace to use `namespace Main::Sub {` syntax
// (instead of `namespace Main { namespace Sub {`)
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
// - Consistent order for args in event bus / slots (Window, View, Model,
// Index)
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
