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
#include <utility>

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

        // TODO: Once New Tab is signal based (if), we can likely drop the
        // interceptor and reroute just here
        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(working_dir / info.relPath) },
              { "title", info.name } },
            window);
    }

    /// TODO SAVES

    // Can call fnxModel_->load to reset DOM snapshot (and might be a better
    // idea, since it ensures DOM is consistent with newly saved Model.xml, BUT
    // it will cause our expanded items to collapse

    // Want to note in docs that if the Notebook archive doesn't exist yet, it
    // will save without running a Save As dialog, because the path will already
    // have been selected when creating the new Notebook (so, Notebook's only
    // Save As dialog is just when selecting Save As)

    // Re: views->fileViews in these: is that fine? Could iterate through
    // provided windows?

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1) return true;
        if (!isModified_()) return true;

        // Is last window
        switch (SavePrompt::exec(fnxPath_.toQString(), window)) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            auto save_result = saveModifiedModels_();

            if (!save_result) {
                colorBars->red();
                auto fail_paths = saveFailDisplayNames_(save_result.failed);
                SaveFailMessageBox::exec(fail_paths, window);

                return false;
            }

            fnxModel_->write(workingDir_.path());

            if (!Fnx::Io::compress(fnxPath_, workingDir_.path())) {
                colorBars->red();
                SaveFailMessageBox::exec(fnxPath_.toQString(), window);

                return false;
            }

            fnxModel_->resetSnapshot();
            showModified_();
            // No green color bar (window closing)

            return true;
        }
        case SavePrompt::Discard:
            return true;
        }
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        if (!isModified_()) return true;

        switch (SavePrompt::exec(fnxPath_.toQString(), windows.last())) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            auto save_result = saveModifiedModels_();

            if (!save_result) {
                colorBars->red();
                auto fail_paths = saveFailDisplayNames_(save_result.failed);
                SaveFailMessageBox::exec(fail_paths, windows.last());

                return false;
            }

            fnxModel_->write(workingDir_.path());

            if (!Fnx::Io::compress(fnxPath_, workingDir_.path())) {
                colorBars->red();
                SaveFailMessageBox::exec(fnxPath_.toQString(), windows.last());

                return false;
            }

            fnxModel_->resetSnapshot();
            showModified_();
            // No green color bar (window closing)

            return true;
        }
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

    void setup_()
    {
        // TODO: Keep as fatal?
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

        /// TODO SAVES

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                if (!isModified_()) return;

                auto save_result = saveModifiedModels_();

                if (!save_result) {
                    colorBars->red();
                    auto fail_paths = saveFailDisplayNames_(save_result.failed);
                    SaveFailMessageBox::exec(fail_paths, cmd.context);

                    return;
                }

                fnxModel_->write(workingDir_.path());

                if (!Fnx::Io::compress(fnxPath_, workingDir_.path())) {
                    colorBars->red();
                    SaveFailMessageBox::exec(fnxPath_.toQString(), cmd.context);

                    return;
                }

                fnxModel_->resetSnapshot();
                showModified_();
                colorBars->green();
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE_AS,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                auto new_path = Coco::PathUtil::Dialog::save(
                    cmd.context,
                    Tr::Dialogs::notebookSaveAsCaption(),
                    fnxPath_,
                    Tr::Dialogs::notebookSaveAsFilter());

                if (new_path.isEmpty()) return;

                auto save_result = saveModifiedModels_();

                if (!save_result) {
                    colorBars->red();
                    auto fail_paths = saveFailDisplayNames_(save_result.failed);
                    SaveFailMessageBox::exec(fail_paths, cmd.context);

                    return;
                }

                fnxModel_->write(workingDir_.path());

                if (!Fnx::Io::compress(new_path, workingDir_.path())) {
                    colorBars->red();
                    SaveFailMessageBox::exec(new_path.toQString(), cmd.context);

                    return;
                }

                /// TODO SAVES

                // TODO: Centralize all the things that would need updated when
                // working dir changes, refactor appropriately

                // TODO: Should the temp dir format and name be an FNX utility?
                // Anything else?

                fnxPath_ = new_path;

                auto new_working_dir = TempDir(
                    AppDirs::temp() / (fnxPath_.fileQString() + "~XXXXXX"));
                if (!new_working_dir.isValid())
                    FATAL(
                        "Notebook working directory creation "
                        "failed!");
                auto old_dir = workingDir_.path();
                auto new_dir = new_working_dir.path();

                if (!Coco::PathUtil::copyContents(old_dir, new_dir))
                    FATAL(
                        "Failed to copy old Notebook working directory "
                        "contents from {} to {}!",
                        old_dir,
                        new_dir);

                for (auto& model : files->fileModels()) {
                    if (!model) continue;
                    auto meta = model->meta();
                    if (!meta) continue;
                    meta->setPath(meta->path().rebase(old_dir, new_dir));
                }

                workingDir_ = std::move(new_working_dir);

                settings->setOverrideConfigPath(
                    workingDir_.path() / Constants::CONFIG_FILE_NAME);
                windows->setSubtitle(fnxPath_.fileQString());

                /// TODO SAVES (END)

                fnxModel_->resetSnapshot();
                showModified_();
                colorBars->green();

                // TODO: Update workspace indicator labels (if we keep them)
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

    bool isModified_() const
    {
        return !fnxPath_.exists() || fnxModel_->isModified();
    }

    void showModified_() { windows->setFlagged(isModified_()); }

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
        // Save modified file models. We're using a list here and going by view
        // so any fails will be displayed consistent with UI order
        QList<AbstractFileModel*> modified_models{};

        for (auto& view : views->fileViews()) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (modified_models.contains(model)) continue;

            modified_models << model;
        }

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

            fail_paths << meta->path().toQString();
        }

        return fail_paths;
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

    void
    onFileModelModificationChanged_(AbstractFileModel* fileModel, bool modified)
    {
        auto meta = fileModel->meta();
        if (!meta) return;
        fnxModel_->setFileEdited(Fnx::Io::uuid(meta->path()), modified);
    }
};

} // namespace Fernanda
