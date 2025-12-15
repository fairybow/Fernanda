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

#include "AbstractService.h"
#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "MenuActions.h"
#include "Menus.h"
#include "Tr.h"

namespace Fernanda {

// ...
class NotebookMenuModule : public AbstractService
{
    Q_OBJECT

public:
    NotebookMenuModule(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        // setup_();
    }

    virtual ~NotebookMenuModule() override { TRACER; }

protected:
    virtual void registerBusCommands() override {}

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &NotebookMenuModule::onWindowCreated_);

        connect(bus, &Bus::windowDestroyed, this, [&](Window* window) {
            // TODO: Some or all of this could also be common util functions in
            // Menus.h
            actions_.remove(window);

            /*if (auto cx = activeTabConnections_.take(window);
                !cx.isEmpty()) {
                for (auto& connection : cx)
                    disconnect(connection);
            }*/
        });
    }

private:
    QHash<Window*, NotebookMenuActions> actions_{};

    // TODO: Go through all handler implementations and see which should use
    // this (handlers that don't use a Command parameter)
    void initializeActions_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];

        /// * = implemented

        actions.file.newFile = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_NEW_FILE,
            Tr::nbNewFile()); /// *

        actions.file.newFolder = Menus::makeCmdlessBusAction(
            bus,
            window,
            Commands::NOTEBOOK_NEW_FOLDER,
            Tr::nbNewFolder()); /// *

        actions.file.renameSelected = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_RENAME_SELECTED,
            Tr::nbRenameSelected()); /// *

        actions.file.removeSelected = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_REMOVE_SELECTED,
            Tr::nbRemoveSelected());

        actions.file.importFiles = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_IMPORT_FILES,
            Tr::nbImportFiles()); /// *

        actions.file.openNotepad = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_OPEN_NOTEPAD,
            Tr::nbOpenNotepad()); /// *

        actions.file.save = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_SAVE,
            Tr::nbSave(),
            Menus::Shortcuts::SAVE); /// *

        actions.file.saveAs = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_SAVE_AS,
            Tr::nbSaveAs(),
            Menus::Shortcuts::SAVE_AS); /// *

        actions.file.exportFiles = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_EXPORT_FILES,
            Tr::nbExportFiles());
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;

        initializeActions_(window);
        auto& actions = actions_[window];
        Menus::addNewMenuBar(
            bus,
            window,
            actions.common,
            [&](QMenu* menu) {
                menu->addAction(actions.file.newFile);
                menu->addAction(actions.file.newFolder);
                menu->addAction(actions.file.renameSelected);
                menu->addAction(actions.file.removeSelected);
                menu->addAction(actions.file.importFiles);
                menu->addAction(actions.file.openNotepad);
            },
            [&](QMenu* menu) {
                menu->addAction(actions.file.save);
                menu->addAction(actions.file.saveAs);
                menu->addAction(actions.file.exportFiles);
            });
    }
};

} // namespace Fernanda
