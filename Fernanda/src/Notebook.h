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

    virtual bool tryQuit() override
    {
        // Delegates to the canCloseAllWindows hook
        return windows->closeAll();
    }

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

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1) return true;
        if (fnxPath_.exists() && !fnxModel_->isModified()) return true;

        // Last window and needs saving
        switch (SavePrompt::exec(fnxPath_.toQString(), window)) {
        default:
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save: {
            Coco::Path path = fnxPath_;

            if (!fnxPath_.exists()) {
                path = promptSaveAs_(window);
                if (path.isEmpty()) return false;
            }

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

            // No resetSnapshot, showModified, or green color bar (last
            // window closing)
            return true;
        }
        case SavePrompt::Discard:
            return true;
        }
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        if (fnxPath_.exists() && !fnxModel_->isModified()) return true;

        auto window = windows.last();

        // Needs saving
        switch (SavePrompt::exec(fnxPath_.toQString(), window)) {
        default:
        case SavePrompt::Cancel:
            return false;
        case SavePrompt::Save: {
            Coco::Path path = fnxPath_;

            if (!fnxPath_.exists()) {
                path = promptSaveAs_(window);
                if (path.isEmpty()) return false;
            }

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

            // No resetSnapshot, showModified, or green color bar (all windows
            // closing)
            return true;
        }
        case SavePrompt::Discard:
            return true;
        }
    }

private:
    Coco::Path fnxPath_; // Intended path (may not exist yet)
    TempDir workingDir_; // Working directory name will remain unchanged for
                         // Notebook's lifetime even when changing Notebook name
                         // via Save As

    FnxModel* fnxModel_ = new FnxModel(this);
    NotebookMenuModule* menus_ = new NotebookMenuModule(bus, this);

    static constexpr auto PATHLESS_FILE_ENTRY_FMT_ =
        "Notebook file entries must have an extant path! [{}]";

    void setup_()
    {
        if (!workingDir_.isValid())
            FATAL("Notebook working directory creation failed!");

        menus_->initialize();

        connect(
            treeViews,
            &TreeViewService::treeViewDoubleClicked,
            this,
            &Notebook::onTreeViewDoubleClicked_);

        connect(
            treeViews,
            &TreeViewService::treeViewContextMenuRequested,
            this,
            &Notebook::onTreeViewContextMenuRequested_);

        connect(
            views,
            &ViewService::addTabRequested,
            this,
            [&](Window* window) {
                // Whereas menu and context menu use currently selected TreeView
                // model index, this does not
                newFile_(window);
            });

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

        settings->setOverrideConfigPath(working_dir / "Settings.ini");

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        bus->addCommandHandler(
            Commands::NOTEBOOK_NEW_FILE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // New file will be under selected TreeView model index (or
                // document element if no current index)
                newFile_(cmd.context, treeViews->currentIndex(cmd.context));
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_NEW_FOLDER,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // New folder will be under selected TreeView model index (or
                // document element if no current index)
                newVirtualFolder_(treeViews->currentIndex(cmd.context));
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_IMPORT_FILES,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // Imported files will be under selected TreeView model index
                // (or document element if no current index)
                importFiles_(cmd.context, treeViews->currentIndex(cmd.context));
            });

        bus->addCommandHandler(Commands::NOTEBOOK_OPEN_NOTEPAD, [&] {
            emit openNotepadRequested();
        });

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                if (fnxPath_.exists() && !fnxModel_->isModified()) return;

                Coco::Path path = fnxPath_;
                auto saved_as = false;

                if (!fnxPath_.exists()) {
                    path = promptSaveAs_(cmd.context);
                    if (path.isEmpty()) return;
                    saved_as = true;
                }

                auto save_result = saveModifiedModels_();
                if (!save_result) {
                    colorBars->red();
                    auto fail_paths = saveFailDisplayNames_(save_result.failed);
                    SaveFailMessageBox::exec(fail_paths, cmd.context);

                    return;
                }

                fnxModel_->write(workingDir_.path());

                if (!Fnx::Io::compress(path, workingDir_.path())) {
                    colorBars->red();
                    SaveFailMessageBox::exec(path.toQString(), cmd.context);

                    return;
                }

                if (saved_as) {
                    fnxPath_ = path;
                    windows->setSubtitle(fnxPath_.fileQString());
                }

                fnxModel_->resetSnapshot();
                showModified_();
                colorBars->green();
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_SAVE_AS,
            [&](const Command& cmd) {
                if (!cmd.context) return;

                auto new_path = promptSaveAs_(cmd.context);
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

                fnxPath_ = new_path;
                windows->setSubtitle(fnxPath_.fileQString());

                fnxModel_->resetSnapshot();
                showModified_();
                colorBars->green();
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_EXPORT_SELECTED_FILE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // TODO
                // Exports selected item (TreeView only allows one selected item
                // at a time)
                // - Copy using startDir / FileInfo::name() +
                // FileInfo::relPath().ext() as start dir in prompt
                // - When we've allowed clicking off an item to unselect in
                // TreeView, then we can have different behavior, perhaps, for
                // Export file? (This would overlap, though, with broader
                // export/concat/compile thing)
            });
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, &Notebook::onWindowCreated_);

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &Notebook::onFileModelModificationChanged_);
    }

    // TODO: Trigger rename immediately (maybe)
    void newFile_(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto working_dir = workingDir_.path();
        // If index is invalid, this function adds it to the DOM document
        // element (top-level)
        auto info = fnxModel_->addNewTextFile(working_dir, index);
        if (!info.isValid()) return;
        files->openFilePathIn(window, working_dir / info.relPath, info.name);
    }

    // TODO: Trigger rename immediately (maybe)
    void newVirtualFolder_(const QModelIndex& index = {})
    {
        if (!workingDir_.isValid()) return;
        // If index is invalid, this function adds it to the DOM document
        // element (top-level)
        fnxModel_->addNewVirtualFolder(index);
    }

    void importFiles_(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto fs_paths = Coco::PathUtil::Dialog::files(
            window,
            Tr::nbImportFileCaption(),
            startDir,
            Tr::nbImportFileFilter());

        if (fs_paths.isEmpty()) return;

        auto working_dir = workingDir_.path();
        // If index is invalid, this function adds it to the DOM document
        // element (top-level)
        auto infos = fnxModel_->importTextFiles(working_dir, fs_paths, index);

        for (auto& info : infos) {
            if (!info.isValid()) continue;
            files->openFilePathIn(
                window,
                working_dir / info.relPath,
                info.name);
        }
    }

    void showModified_()
    {
        windows->setFlagged(!fnxPath_.exists() || fnxModel_->isModified());
    }

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

    Coco::Path promptSaveAs_(Window* window) const
    {
        if (!window) return {};

        // Save As start path will always be fnxPath_
        return Coco::PathUtil::Dialog::save(
            window,
            Tr::nbSaveAsCaption(),
            fnxPath_,
            Tr::nbSaveAsFilter());
    }

private slots:
    // TODO: Could remove working dir validity check; also writeModelFile could
    // return bool?
    void onFnxModelDomChanged_()
    {
        // Initial DOM load emission doesn't call this slot
        if (!workingDir_.isValid()) return;

        fnxModel_->write(workingDir_.path());
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
    // ^ Me from the future: But why would we?
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

    // TODO: Notepad should have one, and a lot of the corresponding menu bar
    // items could go to Notepad, too, like rename, remove, collapse, expand,
    // etc. Basically all of them. Though the behaviors will have to be
    // different, since we're dealing with changing actual paths, etc.
    void onTreeViewContextMenuRequested_(
        Window* window,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;

        auto menu = new QMenu(window);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        // Actions that don't need a valid model index
        auto new_file = menu->addAction(Tr::nbNewFile());
        auto new_folder = menu->addAction(Tr::nbNewFolder());

        connect(new_file, &QAction::triggered, this, [&, window, index] {
            newFile_(window, index);
        });

        connect(new_folder, &QAction::triggered, this, [&, index] {
            newVirtualFolder_(index);
        });

        // Actions that DO need a valid model index. Only one model index can be
        // selected at a time!
        if (index.isValid()) {
            menu->addSeparator();

            auto rename = menu->addAction(Tr::nbRename());
            auto remove = menu->addAction(Tr::nbRemove());

            connect(rename, &QAction::triggered, this, [&, index, window] {
                treeViews->rename(window, index);
            });

            connect(remove, &QAction::triggered, this, [&] {
                // TODO
            });

            menu->addSeparator();

            // TODO: Move up
            if (fnxModel_->hasChildren(index)) {
                auto is_expanded = treeViews->isExpanded(window, index);

                auto collapse_or_expand = menu->addAction(
                    is_expanded ? Tr::nbCollapse() : Tr::nbExpand());
                connect(
                    collapse_or_expand,
                    &QAction::triggered,
                    this,
                    [&, is_expanded, index, window] {
                        is_expanded ? treeViews->collapse(window, index)
                                    : treeViews->expand(window, index);
                    });
            }
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
