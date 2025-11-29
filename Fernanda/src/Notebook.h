/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QAction>
#include <QDomDocument>
#include <QDomElement>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QModelIndex>
#include <QObject>
#include <QPalette> // TODO: Temp
#include <QPoint>
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
#include "IService.h"
#include "NotebookMenuModule.h"
#include "SettingsModule.h"
#include "TempDir.h"
#include "Window.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace operating on 7zip archive-based filesystems.
//
// Owns the archive path and working directory. Uses FnxModel's public API
// exclusively, never accesses DOM elements directly.
//
// There can be any number of Notebooks open during the application lifetime.
//
// TODO: Will want window titles to reflect archive name (plus show modified
// state). Will likely need a WinServ command to set all titles and link this to
// a setModified function here
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
    virtual bool canQuit() override { return windows->closeAll(); }

signals:
    void openNotepadRequested();

protected:
    virtual QAbstractItemModel* treeViewModel() override { return fnxModel_; }

    // TODO: Get element by tag/qualified name? (For future, when we have Trash)
    virtual QModelIndex treeViewRootIndex() override
    {
        // The invalid index represents the root document element (<notebook>).
        // TreeView will display its children (the actual files and virtual
        // folders/structure)
        return {};
    }

    virtual void newTab(Window* window) override
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto working_dir = workingDir_.path();
        auto info = fnxModel_->addNewTextFile(working_dir);
        if (!info.isValid()) return;

        // TODO: Once New Tab is signal based (if), we can likely drop the
        // interceptor and reroute just here
        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(working_dir / info.relPath) },
              { "title", info.name } },
            window);
    }

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1) return true;

        // If archive is modified, prompt
        /*if (modified_) {
            switch (ArchiveSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save
                return true;
            case Discard:
                return true;
            }
        }*/

        return true;
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        /*if (modified_) {
            switch (ArchiveSavePrompt) {
            case Cancel:
                return false;
            case Save:
                // save
                return true;
            case Discard:
                return true;
            }
        }*/

        return true;
    }

private:
    Coco::Path fnxPath_;
    QString name_;
    TempDir workingDir_;

    /// TODO NBM
    // TODO: Replace this with an fnxModel_->domModified() &&
    // fileService->anyModified()? Or perhaps store edited state of files in the
    // DOM element attributes (with a default of false always present) and
    // modification state can be tracked by comparing DOM to an original copy?
    // Is that stupid?
    // bool modified_ = false;

    FnxModel* fnxModel_ = new FnxModel(this);
    NotebookMenuModule* menus_ = new NotebookMenuModule(bus, this);

    void setup_()
    {
        // TODO: Keep as fatal?
        if (!workingDir_.isValid())
            FATAL("Notebook temp directory creation failed!");

        menus_->initialize();

        /// TODO NBM
        windows->setSubtitle(name_);

        // Extraction or creation
        auto working_dir = workingDir_.path();

        if (!fnxPath_.exists()) {
            Fnx::Io::makeNewWorkingDir(working_dir);

            /// TODO NBM
            // May or may not want to mark here
            // setModified_(true);

            //...

        } else {
            Fnx::Io::extract(fnxPath_, working_dir);
            // TODO: Verification (comparing Model file elements to content dir
            // files)
        }

        fnxModel_->load(working_dir);

        connect(
            fnxModel_,
            &FnxModel::domChanged,
            this,
            &Notebook::onFnxModelDomChanged_);

        connect(
            fnxModel_,
            &FnxModel::fileRenamed,
            this,
            &Notebook::onFnxModelFileRenamed_);

        //...

        // TODO: Fnx control its own settings name constant?
        auto settings_file = working_dir / Constants::CONFIG_FILE_NAME;
        settings->setOverrideConfigPath(settings_file);

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        bus->addCommandHandler(Commands::NOTEBOOK_OPEN_NOTEPAD, [&] {
            emit openNotepadRequested();
        });

        bus->addCommandHandler(
            Commands::NOTEBOOK_IMPORT_FILE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                if (!workingDir_.isValid()) return;

                auto parent_dir = fnxPath_.parent();
                if (!parent_dir.exists()) return;

                auto fs_paths = Coco::PathUtil::Dialog::files(
                    cmd.context,
                    Tr::Dialogs::notebookImportFileCaption(),
                    parent_dir,
                    Tr::Dialogs::notebookImportFileFilter());

                if (fs_paths.isEmpty()) return;

                auto working_dir = workingDir_.path();
                auto infos = fnxModel_->importTextFiles(working_dir, fs_paths);

                for (auto& info : infos) {
                    if (!info.isValid()) continue;

                    bus->execute(
                        Commands::OPEN_FILE_AT_PATH,
                        { { "path", qVar(working_dir / info.relPath) },
                          { "title", info.name } },
                        cmd.context);
                }
            });
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, &Notebook::onWindowCreated_);

        connect(
            bus,
            &Bus::treeViewDoubleClicked,
            this,
            &Notebook::onTreeViewDoubleClicked_);

        connect(
            bus,
            &Bus::treeViewContextMenuRequested,
            this,
            &Notebook::onTreeViewContextMenuRequested_);

        /// TODO NBM
        connect(bus, &Bus::fileModelModificationChanged, this, [&] {
            // Set DOM element modified or remove attribute
            // - Need to figure out how we're getting DOM to change elements
            // from outside. By path?
            windows->setFlagged(fnxModel_->isModified());
        });
    }

    /// TODO NBM
    /*void setModified_(bool modified)
    {
        modified_ = modified;
        windows->setFlagged(modified);
    }*/

    /// TODO NBM
    // bool isModified() const
    //{
    //     //
    //     return fnxModel_->isModified()
    // }

    void addWorkspaceIndicator_(Window* window)
    {
        if (!window) return;

        auto status_bar = window->statusBar();
        if (!status_bar) return; // <- Shouldn't happen
        auto temp_label = new QLabel;

        // TODO: Temp
        temp_label->setAutoFillBackground(true);
        QPalette palette = temp_label->palette();
        palette.setColor(QPalette::Window, QColor(Qt::cyan));
        temp_label->setPalette(palette);

        temp_label->setText(name_);
        status_bar->addPermanentWidget(temp_label);
    }

private slots:
    // TODO: Could remove working dir validity check; also writeModelFile could
    // return bool?
    void onFnxModelDomChanged_()
    {
        if (!workingDir_.isValid()) return;
        fnxModel_->write(workingDir_.path());

        /// TODO NBM
        // Initial DOM load emission doesn't call this slot, so we're good to
        // set modified on all subsequent emissions here
        // setModified_(true);
        windows->setFlagged(fnxModel_->isModified());
    }

    void onFnxModelFileRenamed_(const FnxModel::FileInfo& info)
    {
        if (!info.isValid() || !workingDir_.isValid()) return;

        files->setPathTitleOverride(
            workingDir_.path() / info.relPath,
            info.name);
    }

    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }

    // TODO: What if we want to handle virtual folders here, too? Could make
    // generic Info instead and give it an "isDir" member?
    void onTreeViewDoubleClicked_(Window* window, const QModelIndex& index)
    {
        // Notepad uses Path::isDir instead. The asymmetry bugs me, but the
        // folders here are virtual. We would still get success, since working
        // dir would be concatenated to an empty path (unless we give dirs
        // UUIDs), but it would be too abstruse

        if (!window || !index.isValid() || !workingDir_.isValid()) return;
        auto info = fnxModel_->fileInfoAt(index);
        if (!info.isValid()) return;

        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(workingDir_.path() / info.relPath) },
              { "title", info.name } },
            window);
    }

    void onTreeViewContextMenuRequested_(
        Window* window,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;

        auto menu = new QMenu(window);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        auto new_folder =
            menu->addAction(Tr::Menus::notebookTreeViewContextNewFolder());

        // TODO: Trigger rename immediately (maybe)
        connect(new_folder, &QAction::triggered, this, [&, index] {
            fnxModel_->addNewVirtualFolder(index);
        });

        // Add rename action (only if clicking on an actual item)
        if (index.isValid()) {
            menu->addSeparator();
            auto rename_action =
                menu->addAction(Tr::Menus::notebookTreeViewContextRename());

            connect(
                rename_action,
                &QAction::triggered,
                this,
                [&, index, window] { treeViews->renameAt(window, index); });
        }

        menu->popup(globalPos);
    }
};

} // namespace Fernanda
