/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAction>
#include <QHash>
#include <QMenu>
#include <QMenuBar>
#include <QObject>

#include "Coco/Bool.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"

namespace Fernanda {

// Creates and manages menu bars, actions, and keyboard shortcuts for Windows,
// dynamically updating menu states based on Workspace context and routing
// actions through the Commander
class MenuModule : public IService
{
    Q_OBJECT

public:
    MenuModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~MenuModule() override = default;

protected:
    struct BaseActions
    {
        QAction* fileNewTab = nullptr;
        QAction* fileNewWindow = nullptr;
        QAction* fileNewNotebook = nullptr;
        QAction* fileOpenNotebook = nullptr;
        QAction* fileCloseWindow = nullptr;
        QAction* fileCloseAllWindows = nullptr;
        QAction* fileQuit = nullptr;

        QAction* settings = nullptr;

        QAction* helpAbout = nullptr;

        struct Toggles
        {
            QAction* fileClose = nullptr;
            QAction* fileCloseAllInWindow = nullptr;
            QAction* fileCloseAll = nullptr;

            QAction* editUndo = nullptr;
            QAction* editRedo = nullptr;
            QAction* editCut = nullptr;
            QAction* editCopy = nullptr;
            QAction* editPaste = nullptr;
            QAction* editDelete = nullptr;
            QAction* editSelectAll = nullptr;

            QAction* viewPreviousTab = nullptr;
            QAction* viewNextTab = nullptr;
            QAction* viewPreviousWindow = nullptr;
            QAction* viewNextWindow = nullptr;
        } toggles;
    };

    QHash<Window*, BaseActions> baseActions{};

    COCO_BOOL(AutoRepeat);

    QAction* make(
        Window* window,
        const QString& commandId,
        const QString& text,
        const QKeySequence& keySequence = {},
        AutoRepeat autoRepeat = AutoRepeat::No)
    {
        if (!window) return nullptr;

        auto action = new QAction(text, window);
        connect(action, &QAction::triggered, window, [=] {
            commander->execute(commandId, {}, window);
        });
        action->setShortcut(keySequence);
        action->setAutoRepeat(autoRepeat);

        return action;
    }

    virtual void initializeWorkspaceActions_(Window* window) = 0;

private:
    void initialize_()
    {
        connect(
            eventBus,
            &EventBus::windowCreated,
            this,
            &MenuModule::onWindowCreated_);

        //...
    }

    void initializeBaseActions_(Window* window)
    {
        if (!window) return;

        BaseActions actions{};

        actions.fileNewTab = make(
            window,
            Commands::NewTab,
            Tr::Menus::fileNewTab(),
            Qt::CTRL | Qt::Key_D);

        actions.fileNewWindow = make(
            window,
            Commands::NewWindow,
            Tr::Menus::fileNewWindow(),
            Qt::CTRL | Qt::Key_W);

        /// How will these connect? Commander/EventBus are owned by the
        /// Workspaces and new Notebooks are opened by Application (sometimes
        /// via Notepad, with the Interceptor)

        actions.fileNewNotebook =
            make(window, "", Tr::Menus::fileNewNotebook());

        actions.fileOpenNotebook =
            make(window, "", Tr::Menus::fileOpenNotebook());

        actions.toggles.fileClose =
            make(window, Calls::CloseView, Tr::Menus::fileClose());

        actions.toggles.fileCloseAllInWindow = make(
            window,
            Calls::CloseWindowViews,
            Tr::Menus::fileCloseAllInWindow());

        actions.toggles.fileCloseAll =
            make(window, Calls::CloseAllViews, Tr::Menus::fileCloseAll());

        actions.fileCloseWindow =
            make(window, Commands::CloseWindow, Tr::Menus::fileCloseWindow());

        actions.fileCloseAllWindows = make(
            window,
            Commands::CloseAllWindows,
            Tr::Menus::fileCloseAllWindows());

        actions.fileQuit = make(
            window,
            Commands::Quit,
            Tr::Menus::fileQuit(),
            Qt::CTRL | Qt::Key_Q);

        actions.toggles.editUndo = make(
            window,
            Commands::Undo,
            Tr::Menus::editUndo(),
            Qt::CTRL | Qt::Key_Z,
            AutoRepeat::Yes);

        actions.toggles.editRedo = make(
            window,
            Commands::Redo,
            Tr::Menus::editRedo(),
            Qt::CTRL | Qt::Key_Y,
            AutoRepeat::Yes);

        actions.toggles.editCut = make(
            window,
            Commands::Cut,
            Tr::Menus::editCut(),
            Qt::CTRL | Qt::Key_X);

        actions.toggles.editCopy = make(
            window,
            Commands::Copy,
            Tr::Menus::editCopy(),
            Qt::CTRL | Qt::Key_C);

        actions.toggles.editPaste = make(
            window,
            Commands::Paste,
            Tr::Menus::editPaste(),
            Qt::CTRL | Qt::Key_V,
            AutoRepeat::Yes);

        actions.toggles.editDelete = make(
            window,
            Commands::Delete,
            Tr::Menus::editDelete(),
            Qt::Key_Delete);

        actions.toggles.editSelectAll = make(
            window,
            Commands::SelectAll,
            Tr::Menus::editSelectAll(),
            Qt::CTRL | Qt::Key_A);

        actions.toggles.viewPreviousTab = make(
            window,
            Commands::PreviousTab,
            Tr::Menus::viewPreviousTab(),
            Qt::ALT | Qt::Key_1);

        actions.toggles.viewNextTab = make(
            window,
            Commands::NextTab,
            Tr::Menus::viewNextTab(),
            Qt::ALT | Qt::Key_2);

        actions.toggles.viewPreviousWindow = make(
            window,
            Commands::PreviousWindow,
            Tr::Menus::viewPreviousWindow(),
            Qt::ALT | Qt::Key_QuoteLeft);

        actions.toggles.viewNextWindow = make(
            window,
            Commands::ViewNextWindow,
            Tr::Menus::viewNextWindow(),
            Qt::ALT | Qt::Key_3);

        actions.settings =
            make(window, Commands::SettingsDialog, Tr::Menus::settings());

        actions.helpAbout =
            make(window, Commands::AboutDialog, Tr::Menus::helpAbout());

        baseActions[window] = actions;
        // setInitialActionStates_(window);
    }

    void setupMenuBar_(Window* window)
    {
        if (!window) return;
        auto& actions = baseActions[window];
        auto menu_bar = new QMenuBar(window);

        auto file_menu = new QMenu(Tr::Menus::file(), menu_bar);
        file_menu->addAction(actions.fileNewTab);
        file_menu->addAction(actions.fileNewWindow);
        file_menu->addSeparator();

        // file_menu->addAction(actions.fileOpen);
        file_menu->addSeparator();

        // file_menu->addAction(actions.toggles.fileSave);
        // file_menu->addAction(actions.toggles.fileSaveAs);
        // file_menu->addAction(actions.toggles.fileSaveWindow);
        // file_menu->addAction(actions.toggles.fileSaveAll);
        file_menu->addSeparator();

        file_menu->addAction(actions.toggles.fileClose);
        file_menu->addAction(actions.toggles.fileCloseAllInWindow);
        file_menu->addAction(actions.toggles.fileCloseAll);
        file_menu->addSeparator();

        file_menu->addAction(actions.fileCloseWindow);
        file_menu->addAction(actions.fileCloseAllWindows);
        file_menu->addSeparator();

        file_menu->addAction(actions.fileQuit);
        menu_bar->addMenu(file_menu);

        auto edit_menu = new QMenu(Tr::Menus::edit(), menu_bar);
        edit_menu->addAction(actions.toggles.editUndo);
        edit_menu->addAction(actions.toggles.editRedo);
        edit_menu->addSeparator();

        edit_menu->addAction(actions.toggles.editCut);
        edit_menu->addAction(actions.toggles.editCopy);
        edit_menu->addAction(actions.toggles.editPaste);
        edit_menu->addAction(actions.toggles.editDelete);
        edit_menu->addSeparator();

        edit_menu->addAction(actions.toggles.editSelectAll);
        menu_bar->addMenu(edit_menu);

        auto view_menu = new QMenu(Tr::Menus::view(), menu_bar);
        view_menu->addAction(actions.toggles.viewPreviousTab);
        view_menu->addAction(actions.toggles.viewNextTab);
        view_menu->addSeparator();

        view_menu->addAction(actions.toggles.viewPreviousWindow);
        view_menu->addAction(actions.toggles.viewNextWindow);
        menu_bar->addMenu(view_menu);

        menu_bar->addAction(actions.settings);

        auto help_menu = new QMenu(Tr::Menus::help(), menu_bar);
        help_menu->addAction(actions.helpAbout);
        menu_bar->addMenu(help_menu);

        window->setMenuBar(menu_bar);
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        initializeBaseActions_(window);
        initializeWorkspaceActions_(window);
        setupMenuBar_(window);
    }
};

} // namespace Fernanda
