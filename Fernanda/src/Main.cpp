#include <QStringList>

#include "Coco/StartCop.h"

#include "Application.h"
#include "BuildMessages.h"
#include "Version.h"

// - Todo: Pointer guard checks on all functions taking pointers (like
// file/window slots)
// - Todo: Change lists to sets where applicable
// - Todo: Ensure ViewService tracks views when tab is moved!
// - Bug: plain text edit commands not logging (because QPTE has its own
// shortcuts that override our menu)
// - Todo: Split files to .h/.cpp when "finished"
// - Todo: Fix all nested namespace to use `namespace Main::Sub {` syntax
// (instead of `namespace Main { namespace Sub {`)
// - Todo: Update all purpose/responsibility notes on each class
// - Todo: A Fernanda::isDebug function. Its return should be defaulted to the
// build type but also toggleable at runtime
// - Todo: Rename Coco file save dialog to save as, also consider making the
// path change a coco return
// - Todo: qMemoryAddress use QObjectPointer concept instead of QObjectDerived?
// - Todo: Open window workspace func just runs open window command (maybe), and
// menu and view services just connect to window created signal--they should be
// order-independent now
// - Todo: Make prev/next window also unminimize the window (which is still
// shown technically, I think)
// - Todo: I would maybe like to see the call site of queries/commands in log
// - Todo: Query results not printing for window sets (likely just not able to
// print Qt containers)
// - Todo: ViewService::onFileModificationChanged runs on every key stroke!
// Might be fine, might not be
// - Todo: Remove nullptr defaults for parentage where it is mandatory
// - Todo: Unified SavePrompt for one or more files (and just show checkboxes on
// > 1)
// - Todo: Ensure for closes we are always iterating backward (windows and
// views)
// - Todo: We get numbers for our enums in Command result print (need some
// way to make the toQString function just deduce whatever from QVariant)
// - Todo: Ensure all pvt/"internal" functions/members have trailing underscore
// - Todo: Ensure all protected functions/members do NOT have trailing
// underscore
// - Todo: Consistent order for args in event bus / slots (Window, View, Model,
// Index)
// - Bug: So, the activate prev/next window menu shortcuts (and any menu
// shortcut) will stop working when the window is minimized. So, when cycling
// through windows, if one remains minimized (currently won't, since we make
// window->activate call showNormal is it's minimized), then the cycle can't
// continue
// - Todo: Go through call/query calls to make sure we're casting directly in
// the call/query template parameter (and not running to<T> on the result)
// - Todo: Wherever there is a window parameter for a command/call, we probably
// want to just use the context instead of the params map
// - Todo: On closing with a save prompt, when save is chosen, we may want a
// delay to show color bar and the tab unflagging as visual confirmation before
// closing (perhaps an API ColorBarModule can plug into to delay)
// - Todo: Responsibility of window control is (seemingly) all over the place?
// If it's only in Workspace, maybe that's fine, but I'm not sure
// - Todo: Prepopulate file name for Save As dialog
// - Todo: File extensions for Save As
// - Todo: Make WindowService::add private
// - Todo: In MenuMod toggles, note what each toggle's conditions are and when
// it should update
// - Todo: Tab dragging validator!
// - Todo: Probably merge calls/commands?
// - Todo: "Scope" the version macros (prepending FERNANDA_)?
// - Todo: Finish event bus signal logging
// - Todo: Redo purpose statements before publishing
// - Todo: Comments on declarations that we want viewable on Intellisense popup
// for things goes above them, otherwise add a space (like for a subheading)
// - Todo: Finish FileServiceSaveHelper::saveAsDialog_
// - Todo: Remove final period on class purpose statements if present
// - Todo: Un-generalize TabWidget
// - Todo: TabWidgetButton::paintEvent
// - Todo: Check if we need active Window signal
// - Todo: Rework/format Coco (again)!
// - Todo: Ensure ColorBar position is included in settings
// - Todo: Reorg EventBus signal naming (like Commander)
// - Todo: const check!
// - Todo: onFileReadied + updating on modification changed, use a singular "set
// tab properties" function?

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
