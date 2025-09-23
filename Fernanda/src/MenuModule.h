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
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"

#include "Bus.h"
#include "Constants.h"
#include "IService.h"
#include "Tr.h"

namespace Fernanda {

// Creates and manages menu bars, actions, and keyboard shortcuts for Windows,
// dynamically updating menu states based on Workspace context and routing
// actions through the Commander
class MenuModule : public IService
{
    Q_OBJECT

public:
    MenuModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
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
            QAction* fileCloseTab = nullptr;
            QAction* fileCloseAllTabsInWindow = nullptr;
            QAction* fileCloseAllTabs = nullptr;

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
            bus->execute(commandId, window);
        });
        action->setShortcut(keySequence);
        action->setAutoRepeat(autoRepeat);

        return action;
    }

    virtual void initializeWorkspaceActions_(Window* window) = 0;

    [[nodiscard]]
    virtual bool addWorkspaceFileOpenActions_(QMenu* fileMenu, Window* window)
    {
        return false;
    }

    [[nodiscard]]
    virtual bool addWorkspaceFileSaveActions_(QMenu* fileMenu, Window* window)
    {
        return false;
    }

    /*[[nodiscard]]
    virtual bool addWorkspaceMiscFileActions_(QMenu* fileMenu, Window* window)
    {
        return false;
    }*/

private:
    void initialize_()
    {
        connect(bus, &Bus::windowCreated, this, &MenuModule::onWindowCreated_);

        //...
    }

    void initializeBaseActions_(Window* window)
    {
        if (!window) return;
        BaseActions actions{};

        /// Add commands but reimplement them one at a time

        // File/New
        actions.fileNewTab = make(window, "", Tr::Menus::Common::fileNewTab());
        actions.fileNewWindow =
            make(window, "", Tr::Menus::Common::fileNewWindow());
        actions.fileNewNotebook =
            make(window, "", Tr::Menus::Common::fileNewNotebook());

        // File/Open
        actions.fileOpenNotebook =
            make(window, "", Tr::Menus::Common::fileOpenNotebook());

        // File/Close
        actions.toggles.fileCloseTab =
            make(window, "", Tr::Menus::Common::fileCloseTab());
        actions.toggles.fileCloseAllTabsInWindow =
            make(window, "", Tr::Menus::Common::fileCloseAllTabsInWindow());
        actions.toggles.fileCloseAllTabs =
            make(window, "", Tr::Menus::Common::fileCloseAllTabs());
        actions.fileCloseWindow =
            make(window, "", Tr::Menus::Common::fileCloseWindow());
        actions.fileCloseAllWindows =
            make(window, "", Tr::Menus::Common::fileCloseAllWindows());

        // File/Quit
        actions.fileQuit = make(window, "", Tr::Menus::Common::fileQuit());

        // Edit
        actions.toggles.editUndo =
            make(window, "", Tr::Menus::Common::editUndo());
        actions.toggles.editRedo =
            make(window, "", Tr::Menus::Common::editRedo());
        actions.toggles.editCut =
            make(window, "", Tr::Menus::Common::editCut());
        actions.toggles.editCopy =
            make(window, "", Tr::Menus::Common::editCopy());
        actions.toggles.editPaste =
            make(window, "", Tr::Menus::Common::editPaste());
        actions.toggles.editDelete =
            make(window, "", Tr::Menus::Common::editDelete());
        actions.toggles.editSelectAll =
            make(window, "", Tr::Menus::Common::editSelectAll());

        // View
        actions.toggles.viewPreviousTab =
            make(window, "", Tr::Menus::Common::viewPreviousTab());
        actions.toggles.viewNextTab =
            make(window, "", Tr::Menus::Common::viewNextTab());
        actions.toggles.viewPreviousWindow =
            make(window, "", Tr::Menus::Common::viewPreviousWindow());
        actions.toggles.viewNextWindow =
            make(window, "", Tr::Menus::Common::viewNextWindow());

        // Settings
        actions.settings = make(window, "", Tr::Menus::settings());

        // Help
        actions.helpAbout = make(window, "", Tr::Menus::Common::helpAbout());

        baseActions[window] = actions;
        // setInitialToggleStates_(window);
    }

    void setupMenuBar_(Window* window)
    {
        if (!window) return;
        auto& actions = baseActions[window];
        auto menu_bar = new QMenuBar(window);

        setupFileMenu_(window, menu_bar);
        setupEditMenu_(window, menu_bar);
        setupViewMenu_(window, menu_bar);
        setupSettingsMenu_(window, menu_bar);
        setupHelpMenu_(window, menu_bar);

        window->setMenuBar(menu_bar);
    }

    void setupFileMenu_(Window* window, QMenuBar* menuBar)
    {
        if (!window || !menuBar) return;
        auto& actions = baseActions[window];

        auto file_menu = new QMenu(Tr::Menus::file(), menuBar);

        // New
        file_menu->addAction(actions.fileNewTab);
        file_menu->addAction(actions.fileNewWindow);
        file_menu->addAction(actions.fileNewNotebook);
        file_menu->addSeparator();

        // Open
        file_menu->addAction(actions.fileOpenNotebook);
        if (addWorkspaceFileOpenActions_(file_menu, window))
            file_menu->addSeparator();

        // Save
        if (addWorkspaceFileSaveActions_(file_menu, window))
            file_menu->addSeparator();

        // Close
        file_menu->addAction(actions.toggles.fileCloseTab);
        file_menu->addAction(actions.toggles.fileCloseAllTabsInWindow);
        file_menu->addAction(actions.toggles.fileCloseAllTabs);
        file_menu->addSeparator();
        file_menu->addAction(actions.fileCloseWindow);
        file_menu->addAction(actions.fileCloseAllWindows);
        file_menu->addSeparator();

        // Misc
        // if (addWorkspaceMiscFileActions_(file_menu, window))
        // file_menu->addSeparator();

        // Quit
        file_menu->addAction(actions.fileQuit);
        menuBar->addMenu(file_menu);
    }

    void setupEditMenu_(Window* window, QMenuBar* menuBar)
    {
        if (!window || !menuBar) return;
        auto& actions = baseActions[window];

        auto edit_menu = new QMenu(Tr::Menus::edit(), menuBar);
        edit_menu->addAction(actions.toggles.editUndo);
        edit_menu->addAction(actions.toggles.editRedo);
        edit_menu->addSeparator();

        edit_menu->addAction(actions.toggles.editCut);
        edit_menu->addAction(actions.toggles.editCopy);
        edit_menu->addAction(actions.toggles.editPaste);
        edit_menu->addAction(actions.toggles.editDelete);
        edit_menu->addSeparator();

        edit_menu->addAction(actions.toggles.editSelectAll);
        menuBar->addMenu(edit_menu);
    }

    void setupViewMenu_(Window* window, QMenuBar* menuBar)
    {
        if (!window || !menuBar) return;
        auto& actions = baseActions[window];

        auto view_menu = new QMenu(Tr::Menus::view(), menuBar);
        view_menu->addAction(actions.toggles.viewPreviousTab);
        view_menu->addAction(actions.toggles.viewNextTab);
        view_menu->addSeparator();

        view_menu->addAction(actions.toggles.viewPreviousWindow);
        view_menu->addAction(actions.toggles.viewNextWindow);
        menuBar->addMenu(view_menu);
    }

    void setupSettingsMenu_(Window* window, QMenuBar* menuBar)
    {
        if (!window || !menuBar) return;
        auto& actions = baseActions[window];

        menuBar->addAction(actions.settings);
    }

    void setupHelpMenu_(Window* window, QMenuBar* menuBar)
    {
        if (!window || !menuBar) return;
        auto& actions = baseActions[window];

        auto help_menu = new QMenu(Tr::Menus::help(), menuBar);
        help_menu->addAction(actions.helpAbout);
        menuBar->addMenu(help_menu);
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

/// OLD (Remove later):

/*
#pragma once

#include <QAction>
#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <Qt>

#include "Coco/Bool.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Commander.h"
#include "EventBus.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "IService.h"
#include "TabWidget.h"
#include "Tr.h"
#include "Utility.h"
#include "Window.h"

namespace Fernanda {

// Creates and manages menu bars, actions, and keyboard shortcuts for Windows,
// dynamically updating menu states based on Workspace context and routing
// actions through the Commander
class OLDMenuModule : public IService
{
    Q_OBJECT

public:
    OLDMenuModule(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~OLDMenuModule() override = default;

private:
    COCO_BOOL(AutoRepeat);

    struct Actions_
    {
        QAction* fileNewTab = nullptr;
        QAction* fileNewWindow = nullptr;
        QAction* fileOpen = nullptr;
        QAction* fileNewNotebook = nullptr;
        QAction* fileOpenNotebook = nullptr;
        QAction* fileCloseWindow = nullptr;
        QAction* fileCloseAllWindows = nullptr;
        QAction* fileQuit = nullptr;
        QAction* settingsOpen = nullptr;
        QAction* helpAbout = nullptr;

        // These will have their enabled state changed based on Window or
        // Workspace state (could subdivide them further...)
        struct Toggles
        {
            QAction* fileSave = nullptr;
            QAction* fileSaveAs = nullptr;
            QAction* fileSaveWindow = nullptr;
            QAction* fileSaveAll = nullptr;
            QAction* fileClose = nullptr;
            QAction* fileCloseWindowFiles = nullptr;
            QAction* fileCloseAllFiles = nullptr;

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

    QHash<Window*, Actions_> windowActions_{};
    QHash<Window*, QList<QMetaObject::Connection>> activeTabConnections_{};

    void initialize_()
    {
        connect(
            eventBus,
            &EventBus::windowCreated,
            this,
            &OLDMenuModule::onWindowCreated_);

        connect(
            eventBus,
            &EventBus::windowDestroyed,
            this,
            [&](Window* window) {
                windowActions_.remove(window);

                if (auto cx = activeTabConnections_.take(window);
                    !cx.isEmpty()) {
                    for (auto& connection : cx)
                        disconnect(connection);
                }
            });

        connect(
            eventBus,
            &EventBus::fileModificationChanged,
            this,
            &OLDMenuModule::onFileModificationChanged_);

        connect(
            eventBus,
            &EventBus::activeFileViewChanged,
            this,
            &OLDMenuModule::onActiveFileViewChanged_);

        connect(
            eventBus,
            &EventBus::windowTabCountChanged,
            this,
            &OLDMenuModule::onWindowTabCountChanged_);

        connect(
            eventBus,
            &EventBus::visibleWindowCountChanged,
            this,
            &OLDMenuModule::onVisibleWindowCountChanged_);

        connect(
            eventBus,
            &EventBus::viewClosed,
            this,
            &OLDMenuModule::onViewClosed_);
    }

    QAction* makeAction_(
        Window* window,
        const QString& commandId,
        const QString& text,
        AutoRepeat autoRepeat,
        const QKeySequence& keySequence)
    {
        if (!window) return nullptr;

        auto action = new QAction(text, window);
        connect(action, &QAction::triggered, window, [=] {
            commander->execute(commandId, {}, window);
        });
        action->setAutoRepeat(autoRepeat);
        action->setShortcut(keySequence);

        return action;
    }

    void initializeActions_(Window* window)
    {
        if (!window) return;

        auto make = [=](const QString& commandId,
                        const QString& text,
                        const QKeySequence& keySequence = {},
                        AutoRepeat autoRepeat = AutoRepeat::No) {
            return makeAction_(
                window,
                commandId,
                text,
                autoRepeat,
                keySequence);
        };

        Actions_ actions{};

        actions.fileNewTab = make(
            Commands::NewTab,
            Tr::Menus::fileNewTab(),
            Qt::CTRL | Qt::Key_D);

        actions.fileNewWindow = make(
            Commands::NewWindow,
            Tr::Menus::fileNewWindow(),
            Qt::CTRL | Qt::Key_W);

        actions.fileOpen = initializeFileOpenAction_(window);

        /// New Notebook

        /// Open Notebook

        /// Eventually, we may want to handle Call results for saves performed
        /// by the menu

        actions.toggles.fileSave =
            make(Calls::Save, Tr::Menus::fileSave(), Qt::CTRL | Qt::Key_S);

        actions.toggles.fileSaveAs = make(
            Calls::SaveAs,
            Tr::Menus::fileSaveAs(),
            Qt::CTRL | Qt::ALT | Qt::Key_S);

        actions.toggles.fileSaveWindow =
            make(Calls::SaveWindow, Tr::Menus::fileSaveWindow());

        actions.toggles.fileSaveAll = make(
            Calls::SaveAll,
            Tr::Menus::fileSaveAll(),
            Qt::CTRL | Qt::SHIFT | Qt::Key_S);

        actions.toggles.fileClose =
            make(Calls::CloseView, Tr::Menus::fileClose());

        actions.toggles.fileCloseWindowFiles =
            make(Calls::CloseWindowViews, Tr::Menus::fileCloseWindowFiles());

        actions.toggles.fileCloseAllFiles =
            make(Calls::CloseAllViews, Tr::Menus::fileCloseAllFiles());

        actions.fileCloseWindow =
            make(Commands::CloseWindow, Tr::Menus::fileCloseWindow());

        actions.fileCloseAllWindows =
            make(Commands::CloseAllWindows, Tr::Menus::fileCloseAllWindows());

        actions.fileQuit =
            make(Commands::Quit, Tr::Menus::fileQuit(), Qt::CTRL | Qt::Key_Q);

        actions.toggles.editUndo = make(
            Commands::Undo,
            Tr::Menus::editUndo(),
            Qt::CTRL | Qt::Key_Z,
            AutoRepeat::Yes);

        actions.toggles.editRedo = make(
            Commands::Redo,
            Tr::Menus::editRedo(),
            Qt::CTRL | Qt::Key_Y,
            AutoRepeat::Yes);

        actions.toggles.editCut =
            make(Commands::Cut, Tr::Menus::editCut(), Qt::CTRL | Qt::Key_X);

        actions.toggles.editCopy =
            make(Commands::Copy, Tr::Menus::editCopy(), Qt::CTRL | Qt::Key_C);

        actions.toggles.editPaste = make(
            Commands::Paste,
            Tr::Menus::editPaste(),
            Qt::CTRL | Qt::Key_V,
            AutoRepeat::Yes);

        actions.toggles.editDelete =
            make(Commands::Delete, Tr::Menus::editDelete(), Qt::Key_Delete);

        actions.toggles.editSelectAll = make(
            Commands::SelectAll,
            Tr::Menus::editSelectAll(),
            Qt::CTRL | Qt::Key_A);

        actions.toggles.viewPreviousTab = make(
            Commands::PreviousTab,
            Tr::Menus::viewPreviousTab(),
            Qt::ALT | Qt::Key_1);

        actions.toggles.viewNextTab = make(
            Commands::NextTab,
            Tr::Menus::viewNextTab(),
            Qt::ALT | Qt::Key_2);

        actions.toggles.viewPreviousWindow = make(
            Commands::PreviousWindow,
            Tr::Menus::viewPreviousWindow(),
            Qt::ALT | Qt::Key_QuoteLeft);

        actions.toggles.viewNextWindow = make(
            Commands::ViewNextWindow,
            Tr::Menus::viewNextWindow(),
            Qt::ALT | Qt::Key_3);

        actions.settingsOpen =
            make(Commands::SettingsDialog, Tr::Menus::settingsOpen());

        actions.helpAbout = make(Commands::AboutDialog, Tr::Menus::helpAbout());

        windowActions_[window] = actions;
        setInitialActionStates_(window);
    }

    QAction* initializeFileOpenAction_(Window* window)
    {
        if (!window) return nullptr;

        auto action = new QAction(Tr::Menus::fileOpen(), window);
        connect(action, &QAction::triggered, window, [=] {
            auto path = openFilePathDialog_(window);
            if (path.isEmpty() || !path.exists()) return;

            commander->execute(
                Commands::OpenFile,
                { { "path", path.toQString() } },
                window);
        });

        action->setAutoRepeat(false);
        action->setShortcut(Qt::CTRL | Qt::Key_E);

        return action;
    }

    void setInitialActionStates_(Window* window)
    {
        auto active_view = commander->query<IFileView*>(
            Queries::ActiveFileView,
            { { "window", toQVariant(window) } });
        onActiveFileViewChanged_(active_view, window);

        auto tab_count = 0;
        if (auto tab_widget = tabWidget(window))
            tab_count = tab_widget->count();
        onWindowTabCountChanged_(window, tab_count);

        onVisibleWindowCountChanged_(
            commander->query<int>(Queries::VisibleWindowCount));
    }

    Coco::Path openFilePathDialog_(Window* window) const
    {
        auto path = Coco::PathUtil::Dialog::file(
            window,
            Tr::Dialogs::openFileCaption(),
            commander->query<QString>(Queries::NotepadBaseDir));

        return path;
    }

    void setupMenuBar_(Window* window)
    {
        if (!window) return;
        auto& actions = windowActions_[window];
        auto menu_bar = new QMenuBar(window);

        auto file_menu = new QMenu(Tr::Menus::file(), menu_bar);
        file_menu->addAction(actions.fileNewTab);
        file_menu->addAction(actions.fileNewWindow);
        file_menu->addAction(actions.fileOpen);
        file_menu->addSeparator();
        file_menu->addAction(actions.toggles.fileSave);
        file_menu->addAction(actions.toggles.fileSaveAs);
        file_menu->addAction(actions.toggles.fileSaveWindow);
        file_menu->addAction(actions.toggles.fileSaveAll);
        file_menu->addSeparator();
        file_menu->addAction(actions.toggles.fileClose);
        file_menu->addAction(actions.toggles.fileCloseWindowFiles);
        file_menu->addAction(actions.toggles.fileCloseAllFiles);
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

        menu_bar->addAction(actions.settingsOpen);

        auto help_menu = new QMenu(Tr::Menus::help(), menu_bar);
        help_menu->addAction(actions.helpAbout);
        menu_bar->addMenu(help_menu);

        window->setMenuBar(menu_bar);
    }

    void disconnectOldActiveView_(Window* window)
    {
        if (!window) return;

        if (auto old_cx = activeTabConnections_.take(window);
            !old_cx.isEmpty()) {
            for (auto& connection : old_cx)
                disconnect(connection);
        }
    }

    void connectNewActiveView_(Window* window, IFileView* view)
    {
        if (!window || !view) return;
        auto model = view->model();
        if (!model) return;

        auto& cx = activeTabConnections_[window];

        cx << connect(
            model,
            &IFileModel::modificationChanged,
            this,
            [=](bool modified) {
                updatePerViewSaveStates_(window, model);
                updatePerWindowMenuStates_(window, qMax(tabCount(window), 0));
                updatePerWorkspaceMenuStates_(
                    commander->query<int>(Queries::VisibleWindowCount));
            });

        cx << connect(
            model,
            &IFileModel::undoAvailable,
            this,
            [=](bool available) {
                // Maybe add nullptr checks later?
                auto& toggles = windowActions_[window].toggles;
                toggles.editUndo->setEnabled(available);
            });

        cx << connect(
            model,
            &IFileModel::redoAvailable,
            this,
            [=](bool available) {
                auto& toggles = windowActions_[window].toggles;
                toggles.editRedo->setEnabled(available);
            });

        cx << connect(view, &IFileView::selectionChanged, this, [=] {
            auto& toggles = windowActions_[window].toggles;
            toggles.editCut->setEnabled(view->hasSelection());
            toggles.editCopy->setEnabled(view->hasSelection());
            toggles.editDelete->setEnabled(view->hasSelection());
        });

        cx << connect(view, &IFileView::clipboardDataChanged, this, [=] {
            auto& toggles = windowActions_[window].toggles;
            toggles.editPaste->setEnabled(view->hasPaste());
        });
    }

    // Accepts nullptr View!
    void updatePerViewMenuStates_(Window* window, IFileView* view)
    {
        if (!window) return;
        auto& toggles = windowActions_[window].toggles;
        auto model = view ? view->model() : nullptr;

        updatePerViewSaveStates_(window, model);
        toggles.fileClose->setEnabled(view);

        toggles.editUndo->setEnabled(model && model->hasUndo());
        toggles.editRedo->setEnabled(model && model->hasRedo());
        toggles.editCut->setEnabled(view && view->hasSelection());
        toggles.editCopy->setEnabled(view && view->hasSelection());
        toggles.editPaste->setEnabled(view && view->hasPaste());
        toggles.editDelete->setEnabled(view && view->hasSelection());
        toggles.editSelectAll->setEnabled(view && view->supportsEditing());
    }

    void updatePerViewSaveStates_(Window* window, IFileModel* model)
    {
        if (!window) return;
        auto& toggles = windowActions_[window].toggles;

        toggles.fileSave->setEnabled(model && model->isModified());
        toggles.fileSaveAs->setEnabled(model && model->supportsModification());
    }

    void updatePerWindowMenuStates_(Window* window, int count)
    {
        if (!window) return;
        auto& toggles = windowActions_[window].toggles;

        toggles.fileSaveWindow->setEnabled(commander->query<bool>(
            Queries::WindowAnyViewsOnModifiedFiles,
            { { "window", toQVariant(window) } }));
        toggles.fileCloseWindowFiles->setEnabled(commander->query<bool>(
            Queries::WindowAnyFiles,
            { { "window", toQVariant(window) } }));

        auto has_multiple_tabs = count > 1;
        toggles.viewPreviousTab->setEnabled(has_multiple_tabs);
        toggles.viewNextTab->setEnabled(has_multiple_tabs);

        /// Possibly split these actions up later (like with per-view save
        /// actions update)
    }

    void updatePerWorkspaceMenuStates_(int count)
    {
        auto has_multiple_windows = count > 1;
        auto workspace_has_modified_files =
            commander->query<bool>(Queries::WorkspaceAnyViewsOnModifiedFiles);
        auto workspace_has_any_files =
            commander->query<bool>(Queries::WorkspaceAnyFiles);

        for (auto& actions : windowActions_) {
            auto& toggles = actions.toggles;

            toggles.fileSaveAll->setEnabled(workspace_has_modified_files);
            toggles.fileCloseAllFiles->setEnabled(workspace_has_any_files);

            toggles.viewPreviousWindow->setEnabled(has_multiple_windows);
            toggles.viewNextWindow->setEnabled(has_multiple_windows);
        }
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        initializeActions_(window);
        setupMenuBar_(window);
    }

    void onFileModificationChanged_(IFileModel* model, bool modified)
    {
        (void)model;
        (void)modified;

        // Update Window and Workspace states for all Windows
        // since we don't know which Window(s) contain this model
        for (auto& window : commander->query<QSet<Window*>>(Queries::WindowSet))
            updatePerWindowMenuStates_(window, qMax(tabCount(window), 0));

        updatePerWorkspaceMenuStates_(
            commander->query<int>(Queries::VisibleWindowCount));
    }

    // Active View can be nullptr (but will not connect)!
    void onActiveFileViewChanged_(IFileView* view, Window* window)
    {
        if (!window) return;

        updatePerViewMenuStates_(window, view);
        disconnectOldActiveView_(window);
        connectNewActiveView_(window, view);
    }

    void onWindowTabCountChanged_(Window* window, int count)
    {
        if (!window) return;
        updatePerWindowMenuStates_(window, count);
    }

    void onVisibleWindowCountChanged_(int count)
    {
        updatePerWorkspaceMenuStates_(count);
    }

    void onViewClosed_(IFileView* view)
    {
        (void)view;

        // Update Window and Workspace states for all Windows
        // since we don't know which Window(s) contain this model
        for (auto& window : commander->query<QSet<Window*>>(Queries::WindowSet))
            updatePerWindowMenuStates_(window, qMax(tabCount(window), 0));

        updatePerWorkspaceMenuStates_(
            commander->query<int>(Queries::VisibleWindowCount));
    }
};

} // namespace Fernanda
*/
