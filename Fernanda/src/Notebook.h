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
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileModel.h"
#include "AbstractService.h"
#include "AccordionWidget.h"
#include "AppDirs.h"
#include "Bus.h"
#include "CollapsibleWidget.h"
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
#include "TrashPrompt.h"
#include "TreeView.h"
#include "TreeViewService.h"
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
// TODO: Add to docs: trash behavior (still edtiable, tabs do not close when
// moved to trash, still savable; however, emptying trash will close tabs and
// lose unsaved changes)
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

        treeViews->setHeadersHidden(true);

        treeViews->setDockWidgetHook(
            this,
            &Notebook::treeViewDockContentsHook_);

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
                // model index, this does not (and automatically goes to
                // notebook element)
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
            // files, i.e. making sure Trash exists, checking all file UUIDs
            // have corresponding files, etc.)
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
                // notebook element if no current index)
                newFile_(cmd.context, treeViews->currentIndex(cmd.context));
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_NEW_FOLDER,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // New folder will be under selected TreeView model index (or
                // notebook element if no current index)
                newVirtualFolder_(treeViews->currentIndex(cmd.context));
            });

        bus->addCommandHandler(
            Commands::NOTEBOOK_IMPORT_FILES,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                // Imported files will be under selected TreeView model index
                // (or notebook element if no current index)
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

    QModelIndex resolveNotebookIndex_(const QModelIndex& index) const
    {
        return index.isValid() ? index : fnxModel_->notebookIndex();
    }

    // TODO: Trigger rename immediately (maybe)
    void newFile_(Window* window, const QModelIndex& index = {})
    {
        if (!window) return;
        if (!workingDir_.isValid()) return;

        auto working_dir = workingDir_.path();
        // If index is invalid, fnxModel_->addNewTextFile adds it to the DOM
        // document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        auto info = fnxModel_->addNewTextFile(
            working_dir,
            resolveNotebookIndex_(index));
        if (!info.isValid()) return;
        files->openFilePathIn(window, working_dir / info.relPath, info.name);
    }

    // TODO: Trigger rename immediately (maybe)
    void newVirtualFolder_(const QModelIndex& index = {})
    {
        if (!workingDir_.isValid()) return;
        // If index is invalid, fnxModel_->addNewVirtualFolder adds it to the
        // DOM document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        fnxModel_->addNewVirtualFolder(resolveNotebookIndex_(index));
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
        // If index is invalid, fnxModel_->importTextFiles adds it to the DOM
        // document element (top-level), so we make sure it goes to Notebook
        // instead (our root for primary TreeView)
        auto infos = fnxModel_->importTextFiles(
            working_dir,
            fs_paths,
            resolveNotebookIndex_(index));

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

    QWidget* treeViewDockContentsHook_(TreeView* mainTree, Window* window)
    {
        // TODO: Collapse if dragging downward and the widget can't shrink any
        // more?

        auto splitter = new QSplitter(Qt::Vertical, window);
        splitter->addWidget(mainTree);

        auto accordion = new AccordionWidget(window);
        splitter->addWidget(accordion);

        // Trash view
        auto trash_view = new TreeView(window);
        trash_view->setHeaderHidden(true);
        trash_view->setModel(fnxModel_);
        trash_view->setRootIndex(fnxModel_->trashIndex());
        accordion->addWidget(Tr::nbTrash(), trash_view);

        connect(
            trash_view,
            &TreeView::doubleClicked,
            this,
            [this, window](const QModelIndex& index) {
                onTreeViewDoubleClicked_(window, index);
            });

        connect(
            trash_view,
            &TreeView::customContextMenuRequested,
            this,
            [this, window, trash_view](const QPoint& pos) {
                onTrashViewContextMenuRequested_(
                    window,
                    trash_view,
                    trash_view->mapToGlobal(pos),
                    trash_view->indexAt(pos));
            });

        // Test (seems like it works well!)
        // auto test_view = new TreeView(window);
        // test_view->setModel(fnxModel_);
        // test_view->setRootIndex(fnxModel_->trashIndex());
        // accordion->addWidget("Test", test_view);

        // Splitter setup
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);
        splitter->setCollapsible(0, false);
        splitter->setCollapsible(1, false);
        splitter->setHandleWidth(1);

        return splitter;
    }

    void trashPromptAndDelete_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;
        if (!workingDir_.isValid()) return;

        auto file_infos = fnxModel_->fileInfosAt(index);
        if (file_infos.isEmpty()) return;

        if (!TrashPrompt::exec(file_infos.count(), window)) return;

        auto working_dir = workingDir_.path();
        QSet<Coco::Path> paths{};
        for (auto& info : file_infos)
            paths << working_dir / info.relPath;

        auto models = files->modelsFor(paths);

        views->closeViewsForModels(models);
        files->deleteModels(models);

        for (auto& path : paths)
            if (!Coco::PathUtil::remove(path))
                CRITICAL("Failed to delete [{}] from disk!", path);
    }

    void deleteTrashItem_(Window* window, const QModelIndex& index)
    {
        trashPromptAndDelete_(window, index);
        fnxModel_->remove(index);
    }

    void emptyTrash_(Window* window)
    {
        // The trash element itself (tag "trash") isn't a file, so it's skipped
        trashPromptAndDelete_(window, fnxModel_->trashIndex());
        fnxModel_->clearTrash();
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

        auto new_file = menu->addAction(Tr::nbNewFile());
        connect(new_file, &QAction::triggered, this, [&, window, index] {
            newFile_(window, index);
        });

        auto new_folder = menu->addAction(Tr::nbNewFolder());
        connect(new_folder, &QAction::triggered, this, [&, index] {
            newVirtualFolder_(index);
        });

        if (index.isValid()) {
            menu->addSeparator();

            if (fnxModel_->hasChildren(index)) {
                auto is_expanded = treeViews->isExpanded(window, index);

                auto collapse_or_expand = menu->addAction(
                    is_expanded ? Tr::nbCollapse() : Tr::nbExpand());
                connect(
                    collapse_or_expand,
                    &QAction::triggered,
                    this,
                    [&, is_expanded, window, index] {
                        is_expanded ? treeViews->collapse(window, index)
                                    : treeViews->expand(window, index);
                    });
            }

            auto rename = menu->addAction(Tr::nbRename());
            connect(rename, &QAction::triggered, this, [&, window, index] {
                treeViews->edit(window, index);
            });

            menu->addSeparator();

            auto remove = menu->addAction(Tr::nbRemove());
            connect(remove, &QAction::triggered, this, [&, index] {
                fnxModel_->moveToTrash(index);
            });
        }

        menu->popup(globalPos);
    }

    void onTrashViewContextMenuRequested_(
        Window* window,
        TreeView* trashView,
        const QPoint& globalPos,
        const QModelIndex& index)
    {
        if (!window) return;
        if (!fnxModel_->hasTrash()) return;

        auto menu = new QMenu(window);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        if (index.isValid()) {
            if (fnxModel_->hasChildren(index)) {
                auto is_expanded = trashView->isExpanded(index);

                auto collapse_or_expand = menu->addAction(
                    is_expanded ? Tr::nbCollapse() : Tr::nbExpand());
                connect(
                    collapse_or_expand,
                    &QAction::triggered,
                    this,
                    [is_expanded, trashView, index] {
                        is_expanded ? trashView->collapse(index)
                                    : trashView->expand(index);
                    });
            }

            auto rename = menu->addAction(Tr::nbRename());
            connect(rename, &QAction::triggered, this, [&, trashView, index] {
                trashView->edit(index);
            });

            menu->addSeparator();

            auto restore = menu->addAction(Tr::nbRestore());
            connect(restore, &QAction::triggered, this, [&, index] {
                fnxModel_->moveToNotebook_(index);
            });

            auto delete_permanently =
                menu->addAction(Tr::nbDeletePermanently());
            connect(
                delete_permanently,
                &QAction::triggered,
                this,
                [&, window, index] { deleteTrashItem_(window, index); });

            menu->addSeparator();
        }

        auto empty = menu->addAction(Tr::nbEmptyTrash());
        connect(empty, &QAction::triggered, this, [&, window] {
            emptyTrash_(window);
        });

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
