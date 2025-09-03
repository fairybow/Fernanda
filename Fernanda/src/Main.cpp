#include <QStringList>

#include "Coco/StartCop.h"

#include "Application.h"
#include "BuildMessages.h"
#include "Version.h"

// Now/Next:
// - Todo: Tab dragging validator!
// - Todo: Ensure Workspace scoped menu toggles are updated on tab drag
// - Todo: Finish FileServiceSaveHelper::saveAsDialog_
// - Todo: Prepopulate file name for Save As dialog
// - Todo: File extensions for Save As
// - Todo: Ensure ViewService tracks views when tab is moved!

// (Now/Next) Notebook needs:
// - Todo: MenuModule -> NotepadMenuModule (with base MenuModule for both)?
// - Todo: How to handle Notebook paths? VPath wrapper class? We'll need
// something that for Notepad works as a normal path but for Notebooks allows us
// to, when needed (which is most of the time, save its Notepad settings
// fallback path), redirect relative archive paths to OS temp folders, i.e.
// `notebook1-root/file.txt` becomes, under the hood,
// `os-temp-dir/notebook1/file.txt`
// - An Open dialog onto the archive (good god how)
// - Tree view service/module

// Now/Next (But can wait till after Notebook):
// - Todo: How to update SettingsDialog if settings are changed via commander
// from outside?
// - Todo: Ensure ColorBar position is included in settings
// - Todo: On closing with a save prompt, when save is chosen, we may want a
// delay to show color bar and the tab unflagging as visual confirmation before
// closing (perhaps an API ColorBarModule can plug into to delay)

// Minor:
// - Todo: Split files to .h/.cpp when "finished"
// - Todo: Fix all nested namespace to use `namespace Main::Sub {` syntax
// (instead of `namespace Main { namespace Sub {`)
// - Todo: Un-generalize TabWidget
// - Todo: TabWidgetButton::paintEvent
// - Todo: Remove final period on class purpose statements if present
// - Todo: const check!
// - Todo: onFileReadied + updating on modification changed, use a singular "set
// tab properties" function?
// - Todo: I would maybe like to see the call site of queries/commands in log
// - Todo: Remove nullptr defaults for parentage where it is mandatory
// - Bug: So, the activate prev/next window menu shortcuts (and any menu
// shortcut) will stop working when the window is minimized. So, when cycling
// through windows, if one remains minimized (currently won't, since we make
// window->activate call showNormal if it's minimized), then the cycle can't
// continue
// - Todo: Query results not printing for window sets (likely just not able to
// print Qt containers)
// - Todo: A Fernanda::isDebug function. Its return should be defaulted to the
// build type but also toggleable at runtime
// - Todo: ViewService::onFileModificationChanged runs on every key stroke!
// Might be fine, might not be
// - Bug: plain text edit commands not logging (because QPTE has its own
// shortcuts that override our menu)
// - Todo: Pointer guard checks on all functions taking pointers (like
// file/window slots)
// - Todo: Change lists to sets where applicable
// - Todo: We get numbers for our enums in Command result print (need some
// way to make the toQString function just deduce whatever from QVariant)
// - Todo: Unified SavePrompt for one or more files (and just show checkboxes on
// > 1)
// - Todo: Ensure for closes we are always iterating backward (windows and
// views)
// - Todo: Ensure all pvt/"internal" functions/members have trailing underscore
// - Todo: Ensure all protected functions/members do NOT have trailing
// underscore
// - Todo: Consistent order for args in event bus / slots (Window, View, Model,
// Index)
// - Todo: Reorg EventBus signal naming (like Commander)
// - Todo: Ensure all signals are logged in EventBus
// - Todo: Comments on declarations that we want viewable on Intellisense popup
// for things goes above them, otherwise add a space (like for a subheading)
// - Todo: Probably merge calls/commands?
// - Todo: "Scope" the version macros (prepending FERNANDA_)?
// - Todo: Check if we need active Window signal
// - Todo: Make WindowService::add private
// - Todo: In MenuMod toggles, note what each toggle's conditions are and when
// it should update
// - Todo: Go through call/query calls to make sure we're casting directly in
// the call/query template parameter (and not running to<T> on the result)
// - Todo: Wherever there is a window parameter for a command/call, we probably
// want to just use the context instead of the params map

// Coco:
// - Todo: Rework/format Coco (again)!
// - Todo: Rename Coco file save dialog to save as, also consider making the
// path change a coco return
// - Todo: qMemoryAddress use QObjectPointer concept instead of QObjectDerived?

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
