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

#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "IService.h"
#include "MenuActions.h"
#include "Menus.h"
#include "Tr.h"

namespace Fernanda {

// ...
class NotebookMenuModule : public IService
{
    Q_OBJECT

public:
    NotebookMenuModule(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
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

    // TODO: Remove {} for no arg commands
    // TODO: Add key sequences
    void initializeActions_(Window* window)
    {
        if (!window) return;
        auto& actions = actions_[window];

        /// * = implemented

        actions.file.openNotepad = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_OPEN_NOTEPAD,
            Tr::Menus::fileNotebookOpenNotepad());

        actions.file.importFile = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_IMPORT_FILE,
            Tr::Menus::fileNotebookImportFile()); /// *

        actions.file.save = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_SAVE,
            Tr::Menus::fileNotebookSaveArchive());

        actions.file.saveAs = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_SAVE_AS,
            Tr::Menus::fileNotebookSaveArchiveAs());

        actions.file.exportFile = Menus::makeBusAction(
            bus,
            window,
            Commands::NOTEBOOK_EXPORT_FILE,
            Tr::Menus::fileNotebookExportFile());
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;

        initializeActions_(window);
        auto& actions = actions_[window];
        Menus::addNewMenuBar(bus, window, actions.common, [&](QMenu* menu) {
            menu->addSeparator();
            menu->addAction(actions.file.openNotepad);
            menu->addSeparator();
            menu->addAction(actions.file.importFile);
            menu->addSeparator();
            menu->addAction(actions.file.save);
            menu->addAction(actions.file.saveAs);
            menu->addAction(actions.file.exportFile);
            menu->addSeparator();
        });
    }
};

} // namespace Fernanda
