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
#include <QObject>

#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "MenuModule.h"

namespace Fernanda {

// ...
class NotepadMenuModule : public MenuModule
{
    Q_OBJECT

public:
    NotepadMenuModule(Bus* bus, QObject* parent = nullptr)
        : MenuModule(bus, parent)
    {
        initialize_();
    }

    virtual ~NotepadMenuModule() override { TRACER; }

protected:
    virtual void initializeWorkspaceActions_(Window* window) override
    {
        if (!window) return;
        Actions_ actions{};

        /// Add commands but reimplement them one at a time

        // File/Open
        actions.fileOpenFile =
            make(window, "", Tr::Menus::Notepad::fileOpenFile());

        // File/Save
        actions.toggles.fileSaveFile =
            make(window, "", Tr::Menus::Notepad::fileSaveFile());
        actions.toggles.fileSaveFileAs =
            make(window, "", Tr::Menus::Notepad::fileSaveFileAs());
        actions.toggles.fileSaveAllFilesInWindow =
            make(window, "", Tr::Menus::Notepad::fileSaveAllFilesInWindow());
        actions.toggles.fileSaveAllFiles =
            make(window, "", Tr::Menus::Notepad::fileSaveAllFiles());

        actions_[window] = actions;
        // setInitialToggleStates_(window);
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceFileOpenActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.fileOpenFile);
        return true;
    }

    [[nodiscard]]
    virtual bool
    addWorkspaceFileSaveActions_(QMenu* fileMenu, Window* window) override
    {
        if (!fileMenu || !window) return false;
        auto& actions = actions_[window];

        fileMenu->addAction(actions.toggles.fileSaveFile);
        fileMenu->addAction(actions.toggles.fileSaveFileAs);
        fileMenu->addAction(actions.toggles.fileSaveAllFilesInWindow);
        fileMenu->addAction(actions.toggles.fileSaveAllFiles);
        return true;
    }

private:
    struct Actions_
    {
        QAction* fileOpenFile = nullptr;

        struct Toggles
        {
            QAction* fileSaveFile = nullptr;
            QAction* fileSaveFileAs = nullptr;
            QAction* fileSaveAllFilesInWindow = nullptr;
            QAction* fileSaveAllFiles = nullptr;
        } toggles;
    };

    QHash<Window*, Actions_> actions_{};

    void initialize_()
    {
        //...
    }
};

} // namespace Fernanda
