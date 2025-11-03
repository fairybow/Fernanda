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
#include <QModelIndex>
#include <QObject>
#include <QStatusBar>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AppDirs.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "Fnx.h"
#include "FnxModel.h"
#include "NotebookMenuModule.h"
#include "SettingsModule.h"
#include "TempDir.h"
#include "Window.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace that operates on a 7zip archive-based filesystem.
// There can be any number of Notebooks open during the application lifetime
class Notebook : public Workspace
{
    Q_OBJECT

public:
    Notebook(const Coco::Path& fnxPath, QObject* parent = nullptr)
        : Workspace(parent)
        , fnxPath_(fnxPath)
        , name_(fnxPath_.stemQString())
        , workingDir_(AppDirs::temp() / (name_ + "~XXXXXX"))
    {
        setup_();
    }

    virtual ~Notebook() override { TRACER; }

    Coco::Path fnxPath() const noexcept { return fnxPath_; }

private:
    Coco::Path fnxPath_;
    QString name_;
    TempDir workingDir_;

    FnxModel* fnxModel_ = new FnxModel(this);
    NotebookMenuModule* menus_ = new NotebookMenuModule(bus, this);

    void setup_()
    {
        // TODO: Keep as fatal?
        if (!workingDir_.isValid())
            FATAL("Notebook temp directory creation failed!");

        menus_->initialize();

        // Extraction or creation
        auto root = workingDir_.path();

        if (!fnxPath_.exists()) {
            Fnx::makeScaffold(root);
            // TODO: Mark notebook modified (maybe, maybe not until edited)?
            // (need to figure out how this will work)
        } else {
            Fnx::extract(fnxPath_, root);
            // TODO: Verification (comparing Model file elements to content dir
            // files)
        }

        // Read Model.xml into memory as DOM doc
        auto dom = Fnx::readModelXml(root);
        fnxModel_->setDomDocument(dom);

        //...

        auto settings_file = root / Constants::CONFIG_FILE_NAME;
        bus->execute(
            Commands::SET_SETTINGS_OVERRIDE,
            { { "path", qVar(settings_file) } });

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        bus->addCommandHandler(Commands::TREE_VIEW_MODEL, [&] {
            return fnxModel_;
        });

        // TODO: Get element by tag name? (For future, when we have Trash)
        bus->addCommandHandler(Commands::TREE_VIEW_ROOT_INDEX, [&] {
            // The invalid index represents the root document element
            // (<notebook>). TreeView will display its children (the actual
            // files and virtual folders/structure)
            return QModelIndex{};
        });

        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            TRACER;
            qDebug() << "Implement";
        });

        /*bus->addCommandHandler(Cmd::NotebookRoot, [&] {
            return root_.toQString();
        });*/
    }

    void connectBusEvents_()
    {
        // connect(bus, &Bus::windowCreated, this, &Notebook::onWindowCreated_);

        connect(
            bus,
            &Bus::treeViewDoubleClicked,
            this,
            &Notebook::onTreeViewDoubleClicked_);
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
    /*void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }*/

    void onTreeViewDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;

        // Notepad uses Path::isDir instead. The asymmetry bugs me, but the
        // folders here are virtual. We would still get success, since working
        // dir would be concatenated to an empty path (unless we give dirs
        // UUIDs), but it would be too abstruse
        if (fnxModel_->isDir(index)) return;
        auto path = workingDir_.path() / fnxModel_->relativePath(index);

        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(path) } },
            window);
    }
};

} // namespace Fernanda
