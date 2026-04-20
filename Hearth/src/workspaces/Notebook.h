/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>
#include <utility>

#include <QAbstractItemModel>
#include <QDockWidget>
#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QModelIndexList>
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <Coco/Path.h>

#include "core/AppDirs.h"
#include "core/Debug.h"
#include "core/Files.h"
#include "core/Random.h"
#include "core/Tr.h"
#include "menus/MenuBuilder.h"
#include "menus/MenuShortcuts.h"
#include "menus/MenuState.h"
#include "models/AbstractFileModel.h"
#include "nbx/Nbx.h"
#include "nbx/NbxModel.h"
#include "services/AbstractService.h"
#include "services/FileService.h"
#include "services/SettingsService.h"
#include "services/TreeViewService.h"
#include "services/ViewService.h"
#include "services/WindowService.h"
#include "settings/Ini.h"
#include "ui/DrawerWidget.h"
#include "ui/TreeView.h"
#include "ui/Window.h"
#include "workspaces/Backup.h"
#include "workspaces/Bus.h"
#include "workspaces/NotebookColorChip.h"
#include "workspaces/NotebookImport.h"
#include "workspaces/NotebookLockfile.h"
#include "workspaces/SaveFailMessageBox.h"
#include "workspaces/SavePrompt.h"
#include "workspaces/TrashPrompt.h"
#include "workspaces/WorkingDir.h"
#include "workspaces/Workspace.h"

namespace Hearth {

// A binder-style Workspace for working within NBX files.
//
// Owns the archive path and working directory. Uses FnxModel's public API
// exclusively, never accesses DOM elements directly.
//
// There can be any number of Notebooks open during the application lifetime.
//
// TODO: Settings change mark Notebook unsaved? How - watch the working dir for
// changes?
// TODO: Check for missing !workingDir_.isValid checks
/// TODO BA: Should WorkingDir store Dirty UUIDs?
class Notebook : public Workspace
{
    Q_OBJECT

public:
    explicit Notebook(const Coco::Path& nbxPath, QObject* parent = nullptr)
        : Workspace(
              { Ini::LocalKeys::NOTEBOOK_TREE_VIEW_DOCK,
                Ini::LocalKeys::NOTEBOOK_UNIQUE_TABS },
              parent)
        , nbxPath_(nbxPath)
        , workingDir_(newWorkingDirPath_(nbxPath))
    {
        setup_();
    }

    virtual ~Notebook() override
    {
        TRACER;

        clearRecoveryState_(); /// TODO BA
        workingDir_.remove();
    }

    /// TODO BA
    static Notebook*
    recover(const Coco::Path& lockfile, QObject* parent = nullptr)
    {
        auto entry = NotebookLockfile::read(lockfile);
        if (entry.workingDirPath.isEmpty() || !entry.workingDirPath.exists())
            return nullptr;

        if (entry.fnxPath.isEmpty()) return nullptr; // Corrupted lockfile

        return new Notebook(
            entry.fnxPath,
            WorkingDir(entry.workingDirPath),
            std::move(entry.dirtyUuids),
            parent);
    }

    virtual bool tryQuit() override
    {
        // Delegates to the canCloseAllWindows hook
        return windows->closeAll();
    }

    Coco::Path nbxPath() const noexcept { return nbxPath_; }
    QString name() const { return nbxPath_.nameQString(); }

protected:
    virtual void autosave() override
    {
        TRACER;
        writeLockfile_();
    }

    virtual void newFile(Window* window, Files::Type plainTextFileType) override
    {
        newFile_(window, treeViews->currentIndex(window), plainTextFileType);
    }

    virtual void
    importFiles(Window* window, const QList<Coco::Path>& paths) override
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto results = NotebookImport::process(paths);
        if (results.isEmpty()) return;

        auto working_dir_path = workingDir_.path();
        auto parent_index =
            resolveNotebookIndex_(treeViews->currentIndex(window));

        QModelIndex last_index{};

        for (const auto& result : results) {
            auto new_index = nbxModel_->addNewFile(
                result.type,
                result.ext,
                working_dir_path,
                parent_index);
            if (!new_index.isValid()) continue;

            auto info = nbxModel_->fileInfoAt(new_index);
            if (!info.isValid()) continue;

            auto file_path = working_dir_path / info.relPath;
            Io::write(result.content, file_path);

            nbxModel_->setData(new_index, result.suggestedName, Qt::EditRole);
            files->openFilePathIn(window, file_path, result.suggestedName);
            last_index = new_index;
        }

        if (last_index.isValid()) {
            treeViews->expand(window, parent_index);
            treeViews->setCurrentIndex(window, last_index);
        }
    }

    virtual QString importFilter() const override
    {
        return Files::filters(Files::All, Files::conversionImportsFilter());
    }

    virtual QAbstractItemModel* treeViewModel() override { return nbxModel_; }

    virtual QModelIndex treeViewRootIndex() override
    {
        // TreeView displays children of this index, making <notebook> the
        // user-visible root. When nothing is selected, TreeView::currentIndex()
        // returns an invalid QModelIndex.
        //
        // However, FnxModel::elementAt_({}) maps invalid indices to
        // dom_.documentElement(), which is <fnx> (the true DOM root containing
        // both <notebook> and <trash>).
        //
        // This mismatch means Notebook item adding methods must explicitly pass
        // notebookIndex() as a fallback when currentIndex() is invalid,
        // ensuring new files and folders are parented under <notebook> rather
        // than accidentally becoming siblings of <notebook> and <trash>.
        return nbxModel_->notebookIndex();
    }

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1) return true;
        if (nbxPath_.exists() && !nbxModel_->isModified()) return true;

        // Last window and needs saving
        return promptWorkspaceClosingSave_(window);
    }

    virtual bool canCloseAllWindows(const QList<Window*>& windows) override
    {
        if (nbxPath_.exists() && !nbxModel_->isModified()) return true;
        return promptWorkspaceClosingSave_(windows.last());
    }

    virtual void
    fileMenuOpenActions(MenuBuilder& builder, Window* window) override
    {
        /// TODO NF: Move this into the New submenu (and maybe give Notepad new
        /// folder capabilities, meaning only Notepad would override this (for
        /// Open Files... dialog)
        /// TODO NF: ^ However, I'm wary of giving Notepad folder-creation
        builder.action(Tr::nbNewFolder()).onUserTrigger(this, [this, window] {
            newVirtualFolder_(window, treeViews->currentIndex(window));
        });
    }

    virtual void fileMenuSaveActions(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) override
    {
        builder.action(Tr::nxSave())
            .onUserTrigger(this, [this, window] { save_(window); })
            .shortcut(MenuShortcuts::SAVE)
            .enabledToggle(
                state,
                MenuScope::Workspace,
                [this] { return isModified_(); })

            .action(Tr::nxSaveAs())
            .onUserTrigger(this, [this, window] { saveAs_(window); })
            .shortcut(MenuShortcuts::SAVE_AS);
    }

private:
    struct MultiSaveResult_
    {
        QList<AbstractFileModel*> failed{};
        explicit operator bool() const noexcept { return failed.isEmpty(); }
    };

    // Private recovery constructor
    /// TODO BA
    explicit Notebook(
        const Coco::Path& fnxPath,
        WorkingDir&& orphan,
        QSet<QString>&& dirtyUuids,
        QObject* parent = nullptr)
        : Workspace(
              { Ini::LocalKeys::NOTEBOOK_TREE_VIEW_DOCK,
                Ini::LocalKeys::NOTEBOOK_UNIQUE_TABS },
              parent)
        , nbxPath_(fnxPath)
        , workingDir_(std::move(orphan))
        , recoveryDirtyUuids_(std::move(dirtyUuids))
    {
        setup_();
    }

    Coco::Path nbxPath_; // Intended path (may not exist yet)
    WorkingDir workingDir_; // Working directory path/name will remain unchanged
                            // for Notebook's lifetime even when changing
                            // Notebook name via Save As

    FnxModel* nbxModel_ = new FnxModel(this);

    // This should be cleared after the first save or discard
    QSet<QString> recoveryDirtyUuids_{}; /// TODO BA

    static constexpr auto PATHLESS_FILE_ENTRY_FMT_ =
        "Notebook file entries must have an extant path! [{}]";

    QHash<Window*, NotebookColorChip*> colorChips_{};

    static Coco::Path newWorkingDirPath_(const Coco::Path& fnxPath)
    {
        return AppDirs::tempNotebooks()
               / (fnxPath.nameQString() + "~" + Random::token(8));
    }

    void setup_()
    {
        if (!workingDir_.isValid()) {
            FATAL("Notebook working directory creation failed!");
        }

        auto working_dir_path = workingDir_.path();

        treeViews->setHeadersHidden(true);
        treeViews->setDockWidgetHook(this, &Notebook::treeViewDockWidgetHook_);

        connect(
            treeViews,
            &TreeViewService::doubleClicked,
            this,
            &Notebook::onTreeViewsDoubleClicked_);

        connect(
            treeViews,
            &TreeViewService::contextMenuRequested,
            this,
            &Notebook::onTreeViewsContextMenuRequested_);

        connect(
            views,
            &ViewService::addTabRequested,
            this,
            [this](Window* window) {
                // Whereas menu and context menu use currently selected TreeView
                // model index, this does not (and automatically goes to
                // notebook element)
                newFile_(window);
            });

        windows->setSubtitle(name());
        updateWindowsFlags_();

        // Recovery, extraction, or creation
        /// TODO BA
        if (workingDir_.wasAdopted()) {
            // Recovery: working dir already contains autosaved content

        } else if (!nbxPath_.exists()) {
            Nbx::Io::makeNewWorkingDir(working_dir_path);

            //...

        } else {
            Nbx::Io::extract(nbxPath_, working_dir_path);
            // TODO: Verification (comparing Manifest file elements to
            // content dir files, i.e. making sure Trash exists, checking
            // all file UUIDs have corresponding files, etc.)
        }

        settings->setName(name());
        settings->setOverrideConfigPath(
            working_dir_path
            / "Settings.ini"); // This needs to be after extraction!

        nbxModel_->load(working_dir_path);

        connect(
            nbxModel_,
            &FnxModel::domChanged,
            this,
            &Notebook::onFnxModelDomChanged_);

        connect(
            nbxModel_,
            &FnxModel::fileRenamed,
            this,
            &Notebook::onFnxModelFileRenamed_);

        connectBusEvents_();

        /// TODO BA
        if (!recoveryDirtyUuids_.isEmpty()) applyRecoveryState_();
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, [this](Window* window) {
            if (!window) return;
            addColorChip_(window);
        });

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &Notebook::onBusFileModelModificationChanged_);
    }

    void addColorChip_(Window* window)
    {
        if (!window) return;

        // TODO: Tracking/clean-up helper
        auto chip = new NotebookColorChip(nbxPath_);
        colorChips_[window] = chip;
        connect(chip, &QObject::destroyed, this, [this, window] {
            colorChips_.remove(window);
        });

        auto chip_color =
            settings->get<QString>(Ini::LocalKeys::NOTEBOOK_CHIP_COLOR);
        auto text_color =
            settings->get<QString>(Ini::LocalKeys::NOTEBOOK_CHIP_TEXT_COLOR);

        if (!chip_color.isEmpty()) chip->setChipColor(chip_color);
        if (!text_color.isEmpty()) chip->setTextColor(text_color);

        connect(chip, &NotebookColorChip::colorChanged, this, [this, chip] {
            settings->set(
                Ini::LocalKeys::NOTEBOOK_CHIP_COLOR,
                chip->chipColor().name());
            settings->set(
                Ini::LocalKeys::NOTEBOOK_CHIP_TEXT_COLOR,
                chip->textColor().name());
        });

        window->statusBar()->addPermanentWidget(chip);
    }

    /// TODO BA
    Coco::Path lockfilePath_() const
    {
        return NotebookLockfile::path(
            AppDirs::tempNotebookRecovery(),
            workingDir_.path());
    }

    /// TODO BA
    void clearRecoveryState_()
    {
        NotebookLockfile::remove(lockfilePath_());
        recoveryDirtyUuids_.clear();
        files->setAfterModelCreatedHook(nullptr);
    }

    /// TODO BA
    // TODO: If always flushing all dirty models ever becomes a noticable issue,
    // then we may track a "last flushed generation" per model (e.g., comparing
    // QTextDocument::revision() or a hash of model->data() against the last
    // flushed value)
    void writeLockfile_()
    {
        if (!workingDir_.isValid()) return;

        if (!isModified_()) {
            NotebookLockfile::remove(lockfilePath_());
            recoveryDirtyUuids_.clear();
            return;
        }

        QSet<QString> dirty_uuids(recoveryDirtyUuids_);

        for (auto model : files->fileModels()) {
            if (!model || !model->isModified()) continue;
            auto meta = model->meta();
            if (!meta) continue;

            // NB: Uses FileService (not direct Io::write) for watcher
            // suppression. Notebook does not set a beforeWriteHook_, so the
            // backup hook in writeModelToDisk_ is not triggered. If we ever
            // need to use the beforeWriteHook_ in Notebook, then the solution
            // is to have a separate backupHook_ (or similarly named) that
            // Notebook will never need to use, since it does backups via Nbx
            auto result = files->save(model, ClearModified::No);
            if (result != FileService::Success) {
                CRITICAL(
                    "Notebook autosave failed for {} (result: {})!",
                    model,
                    toQString(result));
            }

            dirty_uuids << Nbx::Io::uuid(meta->path());
        }

        NotebookLockfile::write(
            lockfilePath_(),
            nbxPath_,
            workingDir_.path(),
            dirty_uuids);
    }

    /// TODO BA
    void applyRecoveryState_()
    {
        for (auto& uuid : recoveryDirtyUuids_) {
            nbxModel_->setFileEdited(uuid, true);
        }

        files->setAfterModelCreatedHook([this](AbstractFileModel* model) {
            auto meta = model->meta();
            if (!meta) return;
            auto uuid = Nbx::Io::uuid(meta->path());
            if (recoveryDirtyUuids_.contains(uuid)) model->setModified(true);
        });
    }

    bool promptWorkspaceClosingSave_(Window* window)
    {
        if (!window) return false;

        switch (SavePrompt::exec(nbxPath_, window)) {

        default:
        case SavePrompt::Cancel:
            return false;

        case SavePrompt::Save: {
            Coco::Path path = nbxPath_;

            if (!nbxPath_.exists()) {
                path = promptSaveAs_(window);
                if (path.isEmpty()) return false;
            }

            auto save_result = saveModifiedModels_();
            if (!save_result) {
                colorBars->red();
                auto fail_paths = saveFailDisplayPaths_(save_result.failed);
                SaveFailMessageBox::exec(fail_paths, window);

                return false;
            }

            nbxModel_->write(workingDir_.path());

            /// TODO BA
            if (!Nbx::Io::compress(
                    path,
                    workingDir_.path(),
                    makeBackupHook_())) {
                colorBars->red();
                SaveFailMessageBox::exec(path, window);

                return false;
            }

            [[fallthrough]]; // Clean-up after success
        }

        case SavePrompt::Discard:
            // No resetSnapshot, showModified, or green color bar (all windows
            // closing)
            clearRecoveryState_(); /// TODO BA
            return true;
        }
    }

    bool isModified_() const
    {
        return !nbxPath_.exists() || nbxModel_->isModified();
    }

    QModelIndex resolveNotebookIndex_(const QModelIndex& index) const
    {
        return index.isValid() ? index : nbxModel_->notebookIndex();
    }

    // New file will be under selected TreeView model index (or notebook element
    // if no current index)
    /// TODO NF: Make fileType required param?
    void newFile_(
        Window* window,
        const QModelIndex& index = {},
        // TODO: Change to plainTextFileType? Verify that that's all we pass
        // through here?
        Files::Type fileType = Files::PlainText)
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto working_dir_path = workingDir_.path();
        auto parent_index = resolveNotebookIndex_(index);

        auto new_index =
            nbxModel_->addNewFile(fileType, working_dir_path, parent_index);
        if (!new_index.isValid()) return;

        auto info = nbxModel_->fileInfoAt(new_index);
        if (!info.isValid()) return;

        treeViews->expand(window, parent_index);
        treeViews->setCurrentIndex(window, new_index);

        files->openFilePathIn(
            window,
            working_dir_path / info.relPath,
            info.name);

        treeViews->edit(window, new_index);
    }

    // New folder will be under selected TreeView model index (or notebook
    // element if no current index)
    void newVirtualFolder_(Window* window, const QModelIndex& index = {})
    {
        if (!workingDir_.isValid()) return;

        auto parent_index = resolveNotebookIndex_(index);

        auto new_index = nbxModel_->addNewVirtualFolder(parent_index);
        if (!new_index.isValid()) return;

        treeViews->expand(window, parent_index);
        treeViews->setCurrentIndex(window, new_index);
        treeViews->edit(window, new_index);
    }

    /// TODO FT: Export folder
    void exportFile_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;
        if (!workingDir_.isValid()) return;

        auto info = nbxModel_->fileInfoAt(index);
        if (!info.isValid()) return;

        auto source = workingDir_.path() / info.relPath;
        if (!source.exists()) return;

        // Reconstruct original-style filename, e.g. "Chapter One.txt"
        auto suggested_name = info.name + source.extQString();
        auto start_path = currentRootDir / suggested_name;

        auto dest =
            Coco::getSaveFile(window, Tr::nbExportFileCaption(), start_path);

        if (dest.isEmpty()) return;

        if (!Coco::copy(source, dest)) {
            // TODO: error feedback?
            WARN("Failed to export file to {}", dest);
            return;
        }

        colorBars->green(window);
    }

    void updateWindowsFlags_() { windows->setFlagged(isModified_()); }

    MultiSaveResult_ saveModifiedModels_() const
    {
        // No save prompts for Notebook's always-on-disk files
        MultiSaveResult_ result{};

        for (auto& model : files->fileModels()) {
            if (!model || !model->isModified()) continue;

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

        // Unlike in Notepad, we don't get a list of modified models from views
        // in order 0 to n-index by window because models' lives aren't tied to
        // their view in Notebook. We don't really have an inherent order (other
        // than the DOM) so might as well leave unordered or sort alphabetically
        // here
        if (!result.failed.isEmpty()) {
            std::sort(
                result.failed.begin(),
                result.failed.end(),
                [](AbstractFileModel* a, AbstractFileModel* b) {
                    return a->meta()->path() < b->meta()->path();
                });
        }

        return result;
    }

    Coco::PathList
    saveFailDisplayPaths_(const QList<AbstractFileModel*>& failed) const
    {
        if (failed.isEmpty()) return {};

        Coco::PathList fail_paths{};

        for (auto& model : failed) {
            if (!model) continue;
            auto meta = model->meta();
            if (!meta) continue;
            auto path = meta->path();
            if (!path.exists()) FATAL(PATHLESS_FILE_ENTRY_FMT_, path);

            fail_paths << path;
        }

        return fail_paths;
    }

    Coco::Path promptSaveAs_(Window* window) const
    {
        if (!window) return {};

        // Save As start path will always be nbxPath_
        return Coco::getSaveFile(
            window,
            Tr::nbSaveAsCaption(),
            nbxPath_,
            Files::filters(Files::Notebook));
    }

    QWidget* treeViewDockWidgetHook_(TreeView* treeView, Window* window)
    {
        auto splitter = new QSplitter(Qt::Vertical, window);
        splitter->addWidget(treeView);

        // Trash view
        auto trash_view = new TreeView(window);
        trash_view->setHeaderHidden(true);
        trash_view->setModel(nbxModel_);
        trash_view->setRootIndex(nbxModel_->trashIndex());

        auto drawer = new DrawerWidget(Tr::nbTrash(), trash_view, splitter);
        splitter->addWidget(drawer);

        connect(
            trash_view,
            &TreeView::doubleClicked,
            this,
            [this, window](const QModelIndex& index) {
                // Reuse this
                onTreeViewsDoubleClicked_(window, index);
            });

        connect(
            trash_view,
            &TreeView::customContextMenuRequested,
            this,
            [this, window, trash_view](const QPoint& pos) {
                showTrashViewContextMenu_(
                    window,
                    trash_view,
                    trash_view->mapToGlobal(pos),
                    trash_view->indexAt(pos));
            });

        // Splitter setup
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);
        splitter->setCollapsible(0, false);
        splitter->setCollapsible(1, false);
        splitter->setHandleWidth(1);

        return splitter;
    }

    bool trashPromptAndDelete_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return false;
        if (!workingDir_.isValid()) return false;

        auto count = nbxModel_->descendantCount(index);
        if (index != nbxModel_->trashIndex()) ++count;
        if (count == 0) return false;

        if (!TrashPrompt::exec(count, window)) return false;

        auto file_infos = nbxModel_->fileInfosAt(index);
        auto working_dir_path = workingDir_.path();
        QSet<Coco::Path> paths{};

        for (auto& info : file_infos) {
            paths << working_dir_path / info.relPath;
        }

        auto models = files->modelsFor(paths);

        views->closeViewsForModels(models);
        files->deleteModels(models);

        for (auto& path : paths) {
            if (!Coco::remove(path)) {
                CRITICAL("Failed to delete [{}] from disk!", path);
            }
        }

        return true;
    }

    void deleteTrashItem_(Window* window, const QModelIndex& index)
    {
        if (trashPromptAndDelete_(window, index)) nbxModel_->remove(index);
    }

    void restoreTrashItem_(Window* window, const QModelIndex& index)
    {
        auto new_index = nbxModel_->moveToNotebook(index);
        if (!new_index.isValid()) return;
        treeViews->expand(window, new_index.parent());
    }

    void emptyTrash_(Window* window)
    {
        // The trash element itself (tag "trash") isn't a file, so it's skipped
        if (trashPromptAndDelete_(window, nbxModel_->trashIndex())) {
            nbxModel_->clearTrash();
        }
    }

    void save_(Window* window)
    {
        if (!window) return;
        if (nbxPath_.exists() && !nbxModel_->isModified()) return;

        Coco::Path path = nbxPath_;
        auto saved_as = false;

        if (!nbxPath_.exists()) {
            path = promptSaveAs_(window);
            if (path.isEmpty()) return;
            saved_as = true;
        }

        auto save_result = saveModifiedModels_();
        if (!save_result) {
            colorBars->red();
            auto fail_paths = saveFailDisplayPaths_(save_result.failed);
            SaveFailMessageBox::exec(fail_paths, window);

            return;
        }

        nbxModel_->write(workingDir_.path());

        /// TODO BA
        if (!Nbx::Io::compress(path, workingDir_.path(), makeBackupHook_())) {
            colorBars->red();
            SaveFailMessageBox::exec(path, window);

            return;
        }

        clearRecoveryState_(); /// TODO BA

        if (saved_as) {
            nbxPath_ = path;
            windows->setSubtitle(name());
        }

        nbxModel_->resetSnapshot();
        updateWindowsFlags_();
        refreshMenus(MenuScope::Workspace);
        colorBars->green();
    }

    void saveAs_(Window* window)
    {
        if (!window) return;

        auto new_path = promptSaveAs_(window);
        if (new_path.isEmpty()) return;

        auto save_result = saveModifiedModels_();
        if (!save_result) {
            colorBars->red();
            auto fail_paths = saveFailDisplayPaths_(save_result.failed);
            SaveFailMessageBox::exec(fail_paths, window);

            return;
        }

        nbxModel_->write(workingDir_.path());

        /// TODO BA
        if (!Nbx::Io::compress(
                new_path,
                workingDir_.path(),
                makeBackupHook_())) {
            colorBars->red();
            SaveFailMessageBox::exec(new_path, window);

            return;
        }

        clearRecoveryState_(); /// TODO BA

        nbxPath_ = new_path;
        windows->setSubtitle(name());
        for (auto& chip : colorChips_) {
            chip->setFnx(nbxPath_);
        }

        nbxModel_->resetSnapshot();
        updateWindowsFlags_();
        refreshMenus(MenuScope::Workspace);
        colorBars->green();
    }

    void showTrashViewContextMenu_(
        Window* window,
        TreeView* trashView,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;
        if (!nbxModel_->hasTrash()) return;

        auto valid = index.isValid();
        auto has_children = nbxModel_->hasChildren(index);
        auto is_expanded = has_children && trashView->isExpanded(index);

        MenuBuilder(MenuBuilder::ContextMenu, window)
            .actionIf(
                valid && has_children,
                is_expanded ? Tr::nbCollapse() : Tr::nbExpand())
            .onUserTrigger(
                this,
                [is_expanded, trashView, index] {
                    is_expanded ? trashView->collapse(index)
                                : trashView->expand(index);
                })
            .actionIf(valid, Tr::nbRename())
            .onUserTrigger(this, [trashView, index] { trashView->edit(index); })
            .separatorIf(valid)
            .actionIf(valid, Tr::nbRestore())
            .onUserTrigger(
                this,
                [this, window, index] { restoreTrashItem_(window, index); })
            .actionIf(
                valid && nbxModel_->isFile(index),
                Tr::nbExport()) /// TODO FT: Folder export
            .onUserTrigger(
                this,
                [this, window, index] { exportFile_(window, index); })
            .actionIf(valid, Tr::nbDeletePermanently())
            .onUserTrigger(
                this,
                [this, window, index] { deleteTrashItem_(window, index); })
            .separatorIf(valid)
            .action(Tr::nbEmptyTrash())
            .onUserTrigger(this, [this, window] { emptyTrash_(window); })
            .popup(globalPos);
    }

    /// TODO BA
    Nbx::Io::BeforeOverwriteHook makeBackupHook_() const
    {
        return [](const Coco::Path& original) {
            if (!original.exists()) return;
            /// TODO BA: Read from pruning cap from settings?
            Backup::createAndPrune(original, AppDirs::notebookBackups(), 5);
        };
    }

private slots:
    // TODO: Could remove working dir validity check; also writeManifest could
    // return bool?
    void onFnxModelDomChanged_()
    {
        // Initial DOM load emission doesn't call this slot
        if (!workingDir_.isValid()) return;

        nbxModel_->write(workingDir_.path());
        updateWindowsFlags_();
        refreshMenus(MenuScope::Workspace);
    }

    void onFnxModelFileRenamed_(const FnxModel::FileInfo& info)
    {
        if (!info.isValid()) return;
        if (!workingDir_.isValid()) return;

        files->setPathTitleOverride(
            workingDir_.path() / info.relPath,
            info.name);
    }

    // TODO: What if we want to handle virtual folders here, too? Could make
    // generic Info instead and give it an "isDir" member?
    // ^ Me from the future: But why would we?
    void onTreeViewsDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;
        if (!workingDir_.isValid()) return;
        auto info = nbxModel_->fileInfoAt(index);
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
    void onTreeViewsContextMenuRequested_(
        Window* window,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;

        auto valid = index.isValid();
        auto has_children = nbxModel_->hasChildren(index);
        auto is_expanded = has_children && treeViews->isExpanded(window, index);

        MenuBuilder(MenuBuilder::ContextMenu, window)
            .action(Tr::nbNewFile())
            .onUserTrigger(
                this,
                [this, window, index] { newFile_(window, index); })
            .action(Tr::nbNewFolder())
            .onUserTrigger(
                this,
                [this, window, index] { newVirtualFolder_(window, index); })
            .separatorIf(valid)
            .actionIf(
                valid && has_children,
                is_expanded ? Tr::nbCollapse() : Tr::nbExpand())
            .onUserTrigger(
                this,
                [this, is_expanded, window, index] {
                    is_expanded ? treeViews->collapse(window, index)
                                : treeViews->expand(window, index);
                })
            .actionIf(valid, Tr::nbRename())
            .onUserTrigger(
                this,
                [this, window, index] { treeViews->edit(window, index); })
            .separatorIf(valid)
            .actionIf(valid, Tr::nbRemove())
            .onUserTrigger(
                this,
                [this, index] { nbxModel_->moveToTrash(index); })
            .actionIf(
                valid && nbxModel_->isFile(index),
                Tr::nbExport()) /// TODO FT: Folder export
            .onUserTrigger(
                this,
                [this, window, index] { exportFile_(window, index); })
            .popup(globalPos);
    }

    void onBusFileModelModificationChanged_(
        AbstractFileModel* fileModel,
        bool modified)
    {
        if (!fileModel) return;
        auto meta = fileModel->meta();
        if (!meta) return;
        auto path = meta->path();
        if (!path.exists()) FATAL(PATHLESS_FILE_ENTRY_FMT_, path);

        // Notebook's individual archive files should always have a path.
        nbxModel_->setFileEdited(Nbx::Io::uuid(path), modified);

        /// TODO BA:

        if (modified) return;

        auto result = files->save(fileModel, ClearModified::No);
        if (result == FileService::Success) {
            auto uuid = Nbx::Io::uuid(path);
            recoveryDirtyUuids_.remove(uuid);
        } else if (result == FileService::Failure) {
            CRITICAL(
                "Notebook undo-to-clean write-back failed for {} (result: "
                "{})!",
                fileModel,
                toQString(result));
        }
    }
};

} // namespace Hearth
