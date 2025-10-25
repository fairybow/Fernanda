/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QLabel>
#include <QObject>
#include <QStatusBar>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "Fnx.h"
#include "NotebookMenuModule.h"
#include "SettingsModule.h"
#include "TempDir.h"
#include "Utility.h"
#include "Window.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace that operates on a 7zip archive-based filesystem.
// There can be any number of Notebooks open during the application lifetime
class Notebook : public Workspace
{
    Q_OBJECT

public:
    Notebook(
        const Coco::Path& fnxPath,
        const Coco::Path& userDataDir,
        QObject* parent = nullptr)
        : Workspace(userDataDir, parent)
        , fnxPath_(fnxPath)
        , name_(fnxPath_.stemQString())
        , workingDir_(
              userDataDir / Constants::TEMP_DIR_NAME / (name_ + "~XXXXXX"))
    {
        setup_();
    }

    virtual ~Notebook() override { TRACER; }

    Coco::Path fnxPath() const noexcept { return fnxPath_; }

private:
    Coco::Path fnxPath_;
    QString name_;
    TempDir workingDir_;

    NotebookMenuModule* menus_ = new NotebookMenuModule(bus, this);

    void setup_()
    {
        if (!workingDir_.isValid()) {
            CRITICAL("Notebook temp directory creation failed!");
            return;
        }

        menus_->initialize();

        // If path is empty, this is an unsaved Notebook, so we should add the
        // template FNX content to the temp dir. If not, this is existing, and
        // we unpack the archive to temp dir instead.
        if (!fnxPath_.exists()) {
            // create template in temp dir
            // mark notebook modified (need to figure out how this will work)
        } else {
            //
        }

        //...

        auto settings_file = workingDir_.path() / Constants::CONFIG_FILE_NAME;
        bus->execute(
            Commands::SET_SETTINGS_OVERRIDE,
            { { "path", toQVariant(settings_file) } });

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        /*bus->addCommandHandler(Cmd::NotebookRoot, [&] {
            return root_.toQString();
        });*/

        // bus->addCommandHandler(PolyCmd::NEW_TAB, [&](const Command& cmd) {
        //     /// createNewTextFile_(cmd.context); //<- Old (in FileService)
        //     TRACER;
        //     qDebug() << "Implement";
        // });

        // bus->addCommandHandler(PolyCmd::NEW_TREE_VIEW_MODEL, [&] {
        //     // return makeTreeViewModel_();
        //     TRACER;
        //     qDebug() << "Implement";
        // });
    }

    void connectBusEvents_()
    {
        // connect(bus, &Bus::windowCreated, this, &Notebook::onWindowCreated_);
    }

    void addWorkspaceIndicator_(Window* window)
    {
        if (!window) return;

        auto status_bar = window->statusBar();
        if (!status_bar) return; // <- Shouldn't happen
        auto temp_label = new QLabel;
        temp_label->setText(name_);
        status_bar->addPermanentWidget(temp_label);
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }
};

} // namespace Fernanda
