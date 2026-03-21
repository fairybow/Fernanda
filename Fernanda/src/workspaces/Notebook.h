/*
 * Fernanda is a plain text editor for fiction writing
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
#include <QLabel> // TODO: Temp
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPalette> // TODO: Temp
#include <QPoint>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include <Coco/Path.h>

#include "core/AppDirs.h"
#include "core/Debug.h"
#include "core/FileTypes.h"
#include "core/TempDir.h"
#include "core/Tr.h"
#include "fnx/Fnx.h"
#include "fnx/FnxModel.h"
#include "menus/MenuBuilder.h"
#include "menus/MenuShortcuts.h"
#include "menus/MenuState.h"
#include "models/AbstractFileModel.h"
#include "services/AbstractService.h"
#include "services/FileService.h"
#include "services/SettingsService.h"
#include "services/TreeViewService.h"
#include "services/ViewService.h"
#include "services/WindowService.h"
#include "ui/DrawerWidget.h"
#include "ui/TreeView.h"
#include "ui/Window.h"
#include "workspaces/Backup.h"
#include "workspaces/Bus.h"
#include "workspaces/SaveFailMessageBox.h"
#include "workspaces/SavePrompt.h"
#include "workspaces/TrashPrompt.h"
#include "workspaces/Workspace.h"

namespace Fernanda {

// A binder-style Workspace for working within FNX files.
//
// Owns the archive path and working directory. Uses FnxModel's public API
// exclusively, never accesses DOM elements directly.
//
// There can be any number of Notebooks open during the application lifetime.
//
// TODO: Settings change mark Notebook unsaved? How - watch the working dir for
// changes?
class Notebook : public Workspace
{
    Q_OBJECT

public:
    Notebook(const Coco::Path& fnxPath, QObject* parent = nullptr)
        : Workspace(parent)
        , fnxPath_(fnxPath)
        , workingDir_(
              AppDirs::tempNotebooks() / (fnxPath_.nameQString() + "~XXXXXX"))
    {
        setup_();
    }

    virtual ~Notebook() override
    {
        TRACER;
        workingDir_.remove(); // The working directory needs to survive if the
                              // destructor never runs
    }

    Coco::Path fnxPath() const noexcept { return fnxPath_; }

    virtual bool tryQuit() override
    {
        // Delegates to the canCloseAllWindows hook
        return windows->closeAll();
    }

signals:
    void openNotepadRequested();

protected:
    virtual void flushRecoveryData() override
    {
        for (auto model : files->fileModels()) {
            if (!model || !model->isModified()) continue;
            files->save(model, ClearModified::No);
        }

        // - collect uuids of dirty models
        // - write lock file
    }

    virtual QAbstractItemModel* treeViewModel() override { return fnxModel_; }

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
        return fnxModel_->notebookIndex();
    }

    virtual QString treeViewDockIniKey() const override
    {
        return Ini::Keys::NOTEBOOK_TREE_VIEW_DOCK;
    }

    virtual bool canCloseWindow(Window* window) override
    {
        if (windows->count() > 1) return true;
        if (fnxPath_.exists() && !fnxModel_->isModified()) return true;

        // Last window and needs saving
        switch (SavePrompt::exec(fnxPath_, window)) {
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
                auto fail_paths = saveFailDisplayPaths_(save_result.failed);
                SaveFailMessageBox::exec(fail_paths, window);

                return false;
            }

            fnxModel_->write(workingDir_.path());

            if (!Fnx::Io::compress(
                    path,
                    workingDir_.path(),
                    makeBackupHook_())) {
                colorBars->red();
                SaveFailMessageBox::exec(path, window);

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
        switch (SavePrompt::exec(fnxPath_, window)) {
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
                auto fail_paths = saveFailDisplayPaths_(save_result.failed);
                SaveFailMessageBox::exec(fail_paths, window);

                return false;
            }

            fnxModel_->write(workingDir_.path());

            if (!Fnx::Io::compress(
                    path,
                    workingDir_.path(),
                    makeBackupHook_())) {
                colorBars->red();
                SaveFailMessageBox::exec(path, window);

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

    virtual void workspaceMenuHook(
        MenuBuilder& builder,
        MenuState* state,
        Window* window) override
    {
        (void)state;

        builder
            .menu(Tr::notebookMenu())

            .action(Tr::nbOpenNotepad())
            .onUserTrigger(this, [this] { emit openNotepadRequested(); })

            .action(Tr::nbImportFiles())
            .onUserTrigger(this, [this, window] {
                importFiles_(window, treeViews->currentIndex(window));
            });
    }

    virtual void
    fileMenuOpenActions(MenuBuilder& builder, Window* window) override
    {
        builder.action(Tr::nbNewFile())
            .onUserTrigger(
                this,
                [this, window] {
                    newFile_(window, treeViews->currentIndex(window));
                })
            .shortcut(MenuShortcuts::NEW_TAB)

            .action(Tr::nbNewFolder())
            .onUserTrigger(this, [this, window] {
                newVirtualFolder_(treeViews->currentIndex(window));
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
    Coco::Path fnxPath_; // Intended path (may not exist yet)
    TempDir workingDir_; // Working directory name will remain unchanged for
                         // Notebook's lifetime even when changing Notebook name
                         // via Save As

    FnxModel* fnxModel_ = new FnxModel(this);

    static constexpr auto PATHLESS_FILE_ENTRY_FMT_ =
        "Notebook file entries must have an extant path! [{}]";

    void setup_()
    {
        if (!workingDir_.isValid())
            FATAL("Notebook working directory creation failed!");

        workingDir_.setAutoRemove(false);
        auto working_dir_path = workingDir_.path();

        treeViews->setHeadersHidden(true);
        treeViews->setDockWidgetHook(this, &Notebook::treeViewDockWidgetHook_);
        treeViews->setVisibilityConfig(
            treeViewDockIniKey(),
            Ini::Defaults::notebookTreeViewDock()); /// TODO TVT

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

        windows->setSubtitle(fnxPath_.nameQString());
        updateWindowsFlags_();

        // Extraction or creation
        if (!fnxPath_.exists()) {
            Fnx::Io::makeNewWorkingDir(working_dir_path);

            //...

        } else {
            Fnx::Io::extract(fnxPath_, working_dir_path);
            // TODO: Verification (comparing Manifest file elements to content
            // dir files, i.e. making sure Trash exists, checking all file UUIDs
            // have corresponding files, etc.)
        }

        settings->setName(fnxPath_.nameQString());
        settings->setOverrideConfigPath(
            working_dir_path
            / "Settings.ini"); // This needs to be after extraction!

        fnxModel_->load(working_dir_path);

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

        connectBusEvents_();
    }

    void connectBusEvents_()
    {
        connect(bus, &Bus::windowCreated, this, [this](Window* window) {
            // addWorkspaceIndicator_(window);
        });

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &Notebook::onBusFileModelModificationChanged_);
    }

    bool isModified_() const
    {
        return !fnxPath_.exists() || fnxModel_->isModified();
    }

    QModelIndex resolveNotebookIndex_(const QModelIndex& index) const
    {
        return index.isValid() ? index : fnxModel_->notebookIndex();
    }

    // TODO: Trigger rename immediately (maybe)
    // New file will be under selected TreeView model index (or notebook element
    // if no current index)
    // TODO: Later, we'll need to have options for other creatable file types
    // (like markdown, fountain, etc). From a UI perspective, this could be done
    // (in both workspaces) with an overflow menu for new tab, plus a context
    // menu on the add tab button with options
    void newFile_(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto working_dir_path = workingDir_.path();
        // If index is invalid, fnxModel_->addNewTextFile adds it to the DOM
        // document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        auto info = fnxModel_->addNewFile(
            FileTypes::PlainText,
            working_dir_path,
            resolveNotebookIndex_(index));
        if (!info.isValid()) return;

        files->openFilePathIn(
            window,
            working_dir_path / info.relPath,
            info.name);
    }

    // TODO: Trigger rename immediately (maybe)
    // New folder will be under selected TreeView model index (or notebook
    // element if no current index)
    void newVirtualFolder_(const QModelIndex& index = {})
    {
        if (!workingDir_.isValid()) return;
        // If index is invalid, fnxModel_->addNewVirtualFolder adds it to the
        // DOM document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        fnxModel_->addNewVirtualFolder(resolveNotebookIndex_(index));
    }

    // Imported files will be under selected TreeView model index (or notebook
    // element if no current index)
    void importFiles_(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto fs_paths = Coco::getFiles(
            window,
            Tr::nbImportFileCaption(),
            rollingOpenStartDir,
            Tr::nxAllFilesFilter()); /// TODO FT
        if (fs_paths.isEmpty()) return;

        rollingOpenStartDir = fs_paths.at(0).parent();

        auto working_dir_path = workingDir_.path();
        // If index is invalid, fnxModel_->importTextFiles adds it to the DOM
        // document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        auto infos = fnxModel_->importFiles(
            working_dir_path,
            fs_paths,
            resolveNotebookIndex_(index));

        for (auto& info : infos) {
            if (!info.isValid()) continue;

            files->openFilePathIn(
                window,
                working_dir_path / info.relPath,
                info.name);
        }
    }

    /// TODO FT: Export folder
    void exportFile_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;
        if (!workingDir_.isValid()) return;

        auto info = fnxModel_->fileInfoAt(index);
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

    // TODO: Some way to indicate an individual Workspace visually
    void addWorkspaceIndicator_(Window* window)
    {
        if (!window) return;

        // TODO: Temp
        auto status_bar = window->statusBar();
        if (!status_bar) return; // Shouldn't happen
        auto temp_label = new QLabel;
        temp_label->setAutoFillBackground(true);
        QPalette palette = temp_label->palette();
        palette.setColor(QPalette::Window, QColor(Qt::cyan));
        temp_label->setPalette(palette);
        temp_label->setText("Name on open: " + fnxPath_.nameQString());
        status_bar->addPermanentWidget(temp_label);
    }

    struct MultiSaveResult_
    {
        QList<AbstractFileModel*> failed{};
        explicit operator bool() const noexcept { return failed.isEmpty(); }
    };

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

        // Save As start path will always be fnxPath_
        return Coco::getSaveFile(
            window,
            Tr::nbSaveAsCaption(),
            fnxPath_,
            Tr::nbSaveAsFilter());
    }

    QWidget* treeViewDockWidgetHook_(TreeView* treeView, Window* window)
    {
        auto splitter = new QSplitter(Qt::Vertical, window);
        splitter->addWidget(treeView);

        // Trash view
        auto trash_view = new TreeView(window);
        trash_view->setHeaderHidden(true);
        trash_view->setModel(fnxModel_);
        trash_view->setRootIndex(fnxModel_->trashIndex());

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

        auto file_infos = fnxModel_->fileInfosAt(index);
        if (file_infos.isEmpty()) return false;

        if (!TrashPrompt::exec(file_infos.count(), window)) return false;

        auto working_dir_path = workingDir_.path();
        QSet<Coco::Path> paths{};

        for (auto& info : file_infos)
            paths << working_dir_path / info.relPath;

        auto models = files->modelsFor(paths);

        views->closeViewsForModels(models);
        files->deleteModels(models);

        for (auto& path : paths)
            if (!Coco::remove(path))
                CRITICAL("Failed to delete [{}] from disk!", path);

        return true;
    }

    void deleteTrashItem_(Window* window, const QModelIndex& index)
    {
        if (trashPromptAndDelete_(window, index)) fnxModel_->remove(index);
    }

    void emptyTrash_(Window* window)
    {
        // The trash element itself (tag "trash") isn't a file, so it's skipped
        if (trashPromptAndDelete_(window, fnxModel_->trashIndex()))
            fnxModel_->clearTrash();
    }

    void save_(Window* window)
    {
        if (!window) return;
        if (fnxPath_.exists() && !fnxModel_->isModified()) return;

        Coco::Path path = fnxPath_;
        auto saved_as = false;

        if (!fnxPath_.exists()) {
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

        fnxModel_->write(workingDir_.path());

        if (!Fnx::Io::compress(path, workingDir_.path(), makeBackupHook_())) {
            colorBars->red();
            SaveFailMessageBox::exec(path, window);

            return;
        }

        if (saved_as) {
            fnxPath_ = path;
            windows->setSubtitle(fnxPath_.nameQString());
        }

        fnxModel_->resetSnapshot();
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

        fnxModel_->write(workingDir_.path());

        if (!Fnx::Io::compress(
                new_path,
                workingDir_.path(),
                makeBackupHook_())) {
            colorBars->red();
            SaveFailMessageBox::exec(new_path, window);

            return;
        }

        fnxPath_ = new_path;
        windows->setSubtitle(fnxPath_.nameQString());

        fnxModel_->resetSnapshot();
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
        if (!fnxModel_->hasTrash()) return;

        auto valid = index.isValid();
        auto has_children = fnxModel_->hasChildren(index);
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
            .onUserTrigger(
                this,
                [this, trashView, index] { trashView->edit(index); })
            .separatorIf(valid)
            .actionIf(valid, Tr::nbRestore())
            .onUserTrigger(
                this,
                [this, index] { fnxModel_->moveToNotebook(index); })
            .actionIf(
                valid && fnxModel_->isFile(index),
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

    Fnx::Io::BeforeOverwriteHook makeBackupHook_() const
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

        fnxModel_->write(workingDir_.path());
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
    void onTreeViewsContextMenuRequested_(
        Window* window,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;

        auto valid = index.isValid();
        auto has_children = fnxModel_->hasChildren(index);
        auto is_expanded = has_children && treeViews->isExpanded(window, index);

        MenuBuilder(MenuBuilder::ContextMenu, window)
            .action(Tr::nbNewFile())
            .onUserTrigger(
                this,
                [this, window, index] { newFile_(window, index); })
            .action(Tr::nbNewFolder())
            .onUserTrigger(this, [this, index] { newVirtualFolder_(index); })
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
                [this, index] { fnxModel_->moveToTrash(index); })
            .actionIf(
                valid && fnxModel_->isFile(index),
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
        fnxModel_->setFileEdited(Fnx::Io::uuid(path), modified);
    }
};

} // namespace Fernanda
