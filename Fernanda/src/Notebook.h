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
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileModel.h"
#include "AbstractService.h"
#include "AppDirs.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "FileService.h"
#include "Fnx.h"
#include "FnxModel.h"
#include "NotebookMenuModule.h"
#include "SaveFailMessageBox.h"
#include "SavePrompt.h"
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
class Notebook : public Workspace
{
    Q_OBJECT

public:
    Notebook(const Coco::Path& fnxPath, QObject* parent = nullptr)
        : Workspace(parent)
        , fnxPath_(fnxPath)
        , workingDir_(AppDirs::temp() / (fnxPath_.fileQString() + "~XXXXXX"))
    {
        setup_();
    }

    virtual ~Notebook() override { TRACER; }

    Coco::Path fnxPath() const noexcept { return fnxPath_; }
    virtual bool tryQuit() override { return windows->closeAll(); }

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
        files->openFilePathIn(window, working_dir / info.relPath, info.name);
    }

    /// TODO SAVES

    /// EXPORT can use title override + preferred extension!
    /// Or probably better to use extension from DOM...
    ///
    /// Note in docs that Save As will not change working dir name
    ///
    /// NEXT UP: Unfactor save archive and then add the Save As-es to
    /// closures/quit/save
    ///
    /// Can probably just use fnxPath as start path for save as always (it'll
    /// either be real or the startDir / name.fnx

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1 || !isModified_()) return true;

        /// Add Save As

        // Is last window
        switch (SavePrompt::exec(fnxPath_.toQString(), window)) {
        default:
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            return saveArchive_(window);
            // No green color bar (last window closing)
        case SavePrompt::Discard:
            return true;
        }
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        if (!isModified_()) return true;

        /// Add Save As

        auto window = windows.last();

        switch (SavePrompt::exec(fnxPath_.toQString(), window)) {
        default:
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save:
            return saveArchive_(window);
            // No green color bar (all windows closing)
        case SavePrompt::Discard:
            return true;
        }
    }

    /// TODO SAVES (END)

private:
    Coco::Path fnxPath_;
    TempDir workingDir_;

    FnxModel* fnxModel_ = new FnxModel(this);
    NotebookMenuModule* menus_ = new NotebookMenuModule(bus, this);

    static constexpr auto PATHLESS_FILE_ENTRY_FMT_ =
        "Notebook file entries must have an extant path! [{}]";

    void setup_()
    {
        if (!workingDir_.isValid())
            FATAL("Notebook working directory creation failed!");

        menus_->initialize();

        windows->setSubtitle(fnxPath_.fileQString());
        showModified_();

        // Extraction or creation
        auto working_dir = workingDir_.path();

        if (!fnxPath_.exists()) {
            Fnx::Io::makeNewWorkingDir(working_dir);

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

        settings->setOverrideConfigPath(working_dir / "Settings.ini");

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

                auto fs_paths = Coco::PathUtil::Dialog::files(
                    cmd.context,
                    Tr::nbImportFileCaption(),
                    startDir,
                    Tr::nbImportFileFilter());

                if (fs_paths.isEmpty()) return;

                auto working_dir = workingDir_.path();
                auto infos = fnxModel_->importTextFiles(working_dir, fs_paths);

                for (auto& info : infos) {
                    if (!info.isValid()) continue;
                    files->openFilePathIn(
                        cmd.context,
                        working_dir / info.relPath,
                        info.name);
                }
            });

        /// TODO SAVES

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                if (!isModified_()) return;

                if (!saveArchive_(cmd.context)) return;
                colorBars->green();
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE_AS,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                auto new_path = Coco::PathUtil::Dialog::save(
                    cmd.context,
                    Tr::nbSaveAsCaption(),
                    fnxPath_,
                    Tr::nbSaveAsFilter());

                if (new_path.isEmpty()) return;

                /// TODO SAVES

                if (!saveArchive_(cmd.context, new_path)) return;

                // TODO: Should the temp dir format and name be an FNX utility?
                // Anything else?

                fnxPath_ = new_path;
                windows->setSubtitle(fnxPath_.fileQString());

                /// TODO SAVES (END)

                colorBars->green();
            });

        /// TODO SAVES (END)
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

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &Notebook::onFileModelModificationChanged_);
    }

    /// Maybe "unfactor" also - need to Save As if fnxPath_ doesn't exist,
    /// always. Remove to clarify intent/approach
    bool isModified_() const
    {
        return !fnxPath_.exists() || fnxModel_->isModified();
    }

    void showModified_() { windows->setFlagged(isModified_()); }

    void addWorkspaceIndicator_(Window* window)
    {
        if (!window) return;

        auto status_bar = window->statusBar();
        if (!status_bar) return; // Shouldn't happen
        auto temp_label = new QLabel;

        // TODO: Temp
        temp_label->setAutoFillBackground(true);
        QPalette palette = temp_label->palette();
        palette.setColor(QPalette::Window, QColor(Qt::cyan));
        temp_label->setPalette(palette);

        temp_label->setText("Name on open: " + fnxPath_.fileQString());
        status_bar->addPermanentWidget(temp_label);
    }

    /// TODO SAVES

    struct MultiSaveResult_
    {
        QList<AbstractFileModel*> failed{};
        explicit operator bool() const noexcept { return failed.isEmpty(); }
    };

    MultiSaveResult_ saveModifiedModels_() const
    {
        auto modified_models = views->modifiedViewModels();
        if (modified_models.isEmpty()) return {};

        // No save prompts for Notebook's always-on-disk files
        MultiSaveResult_ result{};

        for (auto& model : modified_models) {
            switch (files->save(model)) {
            case FileService::Success:
                break;

            case FileService::NoOp:
            case FileService::Failure:
            default:
                result.failed << model;
                break;
            }
        }

        return result;
    }

    QStringList
    saveFailDisplayNames_(const QList<AbstractFileModel*>& failed) const
    {
        if (failed.isEmpty()) return {};

        QStringList fail_paths{};

        for (auto& model : failed) {
            if (!model) continue;
            auto meta = model->meta();
            if (!meta) continue;
            auto path = meta->path();
            if (!path.exists()) FATAL(PATHLESS_FILE_ENTRY_FMT_, path);

            fail_paths << path.toQString();
        }

        return fail_paths;
    }

    /// "Unfactor"
    bool saveArchive_(Window* window, const Coco::Path& saveAsPath = {})
    {
        Coco::Path path = saveAsPath.isEmpty() ? fnxPath_ : saveAsPath;
        auto save_result = saveModifiedModels_();

        if (!save_result) {
            colorBars->red();
            auto fail_paths = saveFailDisplayNames_(save_result.failed);
            SaveFailMessageBox::exec(fail_paths, window);

            return false;
        }

        fnxModel_->write(workingDir_.path());

        if (!Fnx::Io::compress(path, workingDir_.path())) {
            colorBars->red();
            SaveFailMessageBox::exec(path.toQString(), window);

            return false;
        }

        fnxModel_->resetSnapshot();
        showModified_();

        return true;
    }

    /// TODO SAVES (END)

private slots:
    // TODO: Could remove working dir validity check; also writeModelFile could
    // return bool?
    void onFnxModelDomChanged_()
    {
        if (!workingDir_.isValid()) return;

        fnxModel_->write(workingDir_.path());
        // Initial DOM load emission doesn't call this slot
        showModified_();
    }

    void onFnxModelFileRenamed_(const FnxModel::FileInfo& info)
    {
        if (!info.isValid()) return;
        if (!workingDir_.isValid()) return;

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
        if (!window || !index.isValid()) return;
        if (!workingDir_.isValid()) return;
        auto info = fnxModel_->fileInfoAt(index);
        if (!info.isValid()) return;

        files->openFilePathIn(
            window,
            workingDir_.path() / info.relPath,
            info.name);
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

    void
    onFileModelModificationChanged_(AbstractFileModel* fileModel, bool modified)
    {
        if (!fileModel) return;
        auto meta = fileModel->meta();
        if (!meta) return;
        auto path = meta->path();
        if (!path.exists()) FATAL(PATHLESS_FILE_ENTRY_FMT_, path);

        // Notebook's individual archive files should always have a path.
        fnxModel_->setFileEdited(Fnx::Io::uuid(path), modified);
    }
};

} // namespace Fernanda
