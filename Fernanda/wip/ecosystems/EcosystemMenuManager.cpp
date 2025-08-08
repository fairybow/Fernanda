#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <Qt>

#include "Ecosystem.h"
#include "EcosystemMenuManager.h"
#include "files/FileManager.h"
#include "files/IFile.h"
#include "files/IFileView.h"
#include "tab-view/TabView.h"
#include "Tr.h"
#include "window/Window.h"
#include "window/WindowManager.h"

EcosystemMenuManager::EcosystemMenuManager(Ecosystem* parentEcosystem)
    : QObject(parentEcosystem), eco_(parentEcosystem)
{
}

EcosystemMenuManager::~EcosystemMenuManager() { COCO_TRACER; }

void EcosystemMenuManager::initializeWindowMenus(Window* window)
{
    if (!window) return;

    auto menu_bar = new QMenuBar(window);

    initializeFileMenu_(menu_bar, window);
    initializeEditMenu_(menu_bar, window);
    initializeViewMenu_(menu_bar, window);
    initializeSettingsMenu(menu_bar, window);

    if (eco_->menuPopulator_)
        eco_->menuPopulator_(menu_bar);

    // Initial action states update
    updatePerWindowMenuToggles(window);
    updateEditToggles(window);
    updateViewToggles(window);
    // Global toggle updates occur in Ecosystem, after any potential files have
    // been opened with the new window

    window->setMenuBar(menu_bar);
}

void EcosystemMenuManager::initializeGlobalMenuToggles()
{
    // File menu
    globalToggles_.saveAllInAllWindows = new QAction(Tr::Eco::saveAll(), this);
    globalToggles_.closeAllInAllWindows = new QAction(Tr::Eco::closeAll(), this);
    globalToggles_.saveAllInAllWindows->setAutoRepeat(false);
    globalToggles_.closeAllInAllWindows->setAutoRepeat(false);

    connect
    (
        globalToggles_.saveAllInAllWindows,
        &QAction::triggered,
        this,
        [&] { eco_->runAllColorBarsOnSaveResult_(eco_->saveAllInAllWindows_()); }
    );

    connect
    (
        globalToggles_.closeAllInAllWindows,
        &QAction::triggered,
        this,
        [&] { eco_->closeAllInAllWindows_(); }
    );

    globalToggles_.saveAllInAllWindows->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    //globalToggles_.closeAllInAllWindows->setShortcut();

    // View menu
    globalToggles_.previousWindow = new QAction(Tr::Eco::prevWindow(), this);
    globalToggles_.nextWindow = new QAction(Tr::Eco::nextWindow(), this);
    globalToggles_.previousWindow->setAutoRepeat(false);
    globalToggles_.nextWindow->setAutoRepeat(false);

    connect
    (
        globalToggles_.previousWindow,
        &QAction::triggered,
        this,
        [&] { eco_->windowManager_->activatePrevious(); }
    );

    connect
    (
        globalToggles_.nextWindow,
        &QAction::triggered,
        this,
        [&] { eco_->windowManager_->activateNext(); }
    );

    globalToggles_.previousWindow->setShortcut(Qt::ALT | Qt::Key_QuoteLeft);
    globalToggles_.nextWindow->setShortcut(Qt::ALT | Qt::Key_3);

    updateGlobalMenuToggles();
}

void EcosystemMenuManager::updateGlobalMenuToggles()
{
    if (globalToggles_.saveAllInAllWindows)
        globalToggles_.saveAllInAllWindows->setEnabled(eco_->anySavableFilesInAnyWindow_());

    if (globalToggles_.closeAllInAllWindows)
        globalToggles_.closeAllInAllWindows->setEnabled(eco_->anyCloseableViewsInAnyWindow_());

    auto has_multiple_windows = eco_->windowManager_->count() > 1;

    if (globalToggles_.previousWindow)
        globalToggles_.previousWindow->setEnabled(has_multiple_windows);

    if (globalToggles_.nextWindow)
        globalToggles_.nextWindow->setEnabled(has_multiple_windows);
}

void EcosystemMenuManager::updatePerWindowMenuToggles(Window* window)
{
    if (!window) return;

    auto& actions = perWindowToggles_[window];
    auto active_file_view = eco_->activeFileViewIn_(window);

    auto active_file = active_file_view
        ? active_file_view->model()
        : nullptr;

    if (actions.saveActiveFile)
        actions.saveActiveFile->setEnabled(FileManager::isEdited(active_file));

    if (actions.saveActiveFileAs)
        actions.saveActiveFileAs->setEnabled(FileManager::canSaveAs(active_file));

    if (actions.saveAllInWindow)
        actions.saveAllInWindow->setEnabled(eco_->anySavableFilesIn_(window));

    if (actions.closeActiveFileView)
        actions.closeActiveFileView->setEnabled(active_file_view);

    if (actions.closeAllInWindow)
        actions.closeAllInWindow->setEnabled(eco_->anyCloseableViewsIn_(window));
}

void EcosystemMenuManager::updateEditToggles(Window* window)
{
    if (!window) return;

    auto& actions = perWindowToggles_[window];
    auto active_file_view = eco_->activeFileViewIn_(window);

    auto active_file = active_file_view
        ? active_file_view->model()
        : nullptr;

    // File
    if (actions.undo) actions.undo->setEnabled(FileManager::canUndo(active_file));
    if (actions.redo) actions.redo->setEnabled(FileManager::canRedo(active_file));

    if (actions.cut) actions.cut->setEnabled(FileManager::hasSelection(active_file_view));
    if (actions.copy) actions.copy->setEnabled(FileManager::hasSelection(active_file_view));

    if (actions.paste)
        actions.paste->setEnabled(FileManager::canPaste(active_file_view));

    if (actions.del) actions.del->setEnabled(FileManager::hasSelection(active_file_view));

    if (actions.selectAll)
        actions.selectAll->setEnabled(FileManager::canSelectAll(active_file_view));
}

void EcosystemMenuManager::updateViewToggles(Window* window)
{
    if (!window) return;

    auto& actions = perWindowToggles_[window];

    auto tab_view = eco_->tabViewIn_(window);

    auto has_multiple_tabs = tab_view
        ? tab_view->count() > 1
        : false;

    if (actions.previousTab) actions.previousTab->setEnabled(has_multiple_tabs);
    if (actions.nextTab) actions.nextTab->setEnabled(has_multiple_tabs);
}

void EcosystemMenuManager::cleanup(Window* window)
{
    perWindowToggles_.remove(window);
}

// qApp may handle Quit just fine, if it does what I think it does (just
// call close on each window). Still, we want to be able to potentially
// session in Environment (and save any configs here or there, wherever
// they'll be...)
void EcosystemMenuManager::initializeFileMenu_(QMenuBar* menuBar, Window* window)
{
    if (!menuBar || !window) return;

    auto menu = new QMenu(Tr::Eco::file(), menuBar);

    // Store stateful actions
    auto& actions = perWindowToggles_[window];

    addAction_
    (
        Tr::Eco::newTab(),
        Qt::CTRL | Qt::Key_D,
        menu,
        [=] { eco_->openNewFile_(window); }
    );

    addAction_
    (
        Tr::Eco::newWindow(),
        Qt::CTRL | Qt::Key_W,
        menu,
        [&] { eco_->openWindow(); }
    );

    addAction_
    (
        Tr::Eco::open(),
        Qt::CTRL | Qt::Key_E,
        menu,
        [=] { eco_->openExistingFile_(window); }
    );

    menu->addSeparator();

    actions.saveActiveFile = addAction_
    (
        Tr::Eco::save(),
        Qt::CTRL | Qt::Key_S,
        menu,
        [=] { eco_->runColorBarOnSaveResult_(window, eco_->saveActiveFile_(window)); }
    );

    actions.saveActiveFileAs = addAction_
    (
        Tr::Eco::saveAs(),
        Qt::CTRL | Qt::ALT | Qt::Key_S,
        menu,
        [=] { eco_->runColorBarOnSaveResult_(window, eco_->saveActiveFileAs_(window)); }
    );

    actions.saveAllInWindow = addAction_
    (
        Tr::Eco::saveAllInWindow(),
        menu,
        [=] { eco_->runColorBarOnSaveResult_(window, eco_->saveAllInWindow_(window)); }
    );

    menu->addAction(globalToggles_.saveAllInAllWindows);
    globalToggles_.saveAllInAllWindows->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);

    menu->addSeparator();

    actions.closeActiveFileView = addAction_
    (
        Tr::Eco::close(),
        menu,
        [=] { eco_->onCloseActiveFileView_(window); }
    );

    actions.closeAllInWindow = addAction_
    (
        Tr::Eco::closeAllInWindow(),
        menu,
        [=] { eco_->onCloseAllInWindow_(window); }
    );

    menu->addAction(globalToggles_.closeAllInAllWindows);

    menu->addSeparator();

    // WindowManager's CloseAcceptor will handle closing operations
    addAction_
    (
        Tr::Eco::closeWindow(),
        menu,
        [=] { window->close(); }
    );

    addAction_
    (
        Tr::Eco::closeAllWindows(),
        menu,
        [&] { eco_->windowManager_->closeAll(WindowManager::HaltOnRefusal::Yes); }
    );

    menu->addSeparator();

    addAction_
    (
        Tr::Eco::quit(),
        Qt::CTRL | Qt::Key_Q,
        menu,
        qApp,
        &QApplication::quit,
        Qt::QueuedConnection
    );

    menuBar->addMenu(menu);
}

void EcosystemMenuManager::initializeEditMenu_(QMenuBar* menuBar, Window* window)
{
    if (!menuBar || !window) return;

    auto menu = new QMenu(Tr::Eco::edit(), menuBar);

    // Store stateful actions
    auto& actions = perWindowToggles_[window];

    actions.undo = addAction_
    (
        Tr::Eco::undo(),
        Qt::CTRL | Qt::Key_Z,
        menu,
        [=] { eco_->undoOnActiveFile_(window); }
    );

    actions.redo = addAction_
    (
        Tr::Eco::redo(),
        Qt::CTRL | Qt::Key_Y,
        menu,
        [=] { eco_->redoOnActiveFile_(window); }
    );

    menu->addSeparator();

    actions.cut = addAction_
    (
        Tr::Eco::cut(),
        Qt::CTRL | Qt::Key_X,
        menu,
        [=] { eco_->cutOnActiveFileView_(window); }
    );

    actions.copy = addAction_
    (
        Tr::Eco::copy(),
        Qt::CTRL | Qt::Key_C,
        menu,
        [=] { eco_->copyOnActiveFileView_(window); }
    );

    actions.paste = addAction_
    (
        Tr::Eco::paste(),
        Qt::CTRL | Qt::Key_V,
        menu,
        [=] { eco_->pasteOnActiveFileView_(window); }
    );

    actions.del = addAction_
    (
        Tr::Eco::del(),
        Qt::Key_Delete,
        menu,
        [=] { eco_->deleteOnActiveFileView_(window); }
    );

    menu->addSeparator();

    actions.selectAll = addAction_
    (
        Tr::Eco::selectAll(),
        Qt::CTRL | Qt::Key_A,
        menu,
        [=] { eco_->selectAllOnActiveFileView_(window); }
    );

    actions.undo->setAutoRepeat(true);
    actions.redo->setAutoRepeat(true);
    actions.paste->setAutoRepeat(true);

    menuBar->addMenu(menu);
}

void EcosystemMenuManager::initializeViewMenu_(QMenuBar* menuBar, Window* window)
{
    if (!menuBar || !window) return;

    auto menu = new QMenu(Tr::Eco::view(), menuBar);

    // Store stateful actions
    auto& actions = perWindowToggles_[window];

    actions.previousTab = addAction_
    (
        Tr::Eco::prevTab(),
        Qt::ALT | Qt::Key_1,
        menu,
        [=] { if (auto tab_view = eco_->tabViewIn_(window)) tab_view->activatePrevious(); }
    );

    actions.nextTab = addAction_
    (
        Tr::Eco::nextTab(),
        Qt::ALT | Qt::Key_2,
        menu,
        [=] { if (auto tab_view = eco_->tabViewIn_(window)) tab_view->activateNext(); }
    );

    menu->addSeparator();

    menu->addAction(globalToggles_.previousWindow);
    menu->addAction(globalToggles_.nextWindow);

    menuBar->addMenu(menu);
}

void EcosystemMenuManager::initializeSettingsMenu(QMenuBar* menuBar, Window* window)
{
    if (!menuBar || !window) return;
    auto settings = new QAction(Tr::Eco::settings(), menuBar);
    settings->setAutoRepeat(false);
    menuBar->addAction(settings);
    connect(settings, &QAction::triggered, this, [&] { eco_->openSettingsDialog_(); });
}
