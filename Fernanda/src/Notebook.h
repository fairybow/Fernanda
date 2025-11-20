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
#include "NotebookMenuModule.h"
#include "SettingsModule.h"
#include "TempDir.h"
#include "Window.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace that operates on a 7zip archive-based filesystem.
// There can be any number of Notebooks open during the application lifetime
//
// TODO: Will want window titles to reflect archive name (plus show modified
// state). Will likely need a WinServ command to set all titles and link this to
// a setModified function here
// TODO: Solidify what goes where between Notebook, Fnx, and FnxModel
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

protected:
    /// WIP
    virtual bool canCloseWindow(Window* window) override
    {
        // TODO: Notebook will have to prompt not for tab closes but for window
        // closures (if last window) and app quit

        if (!window) return false;

        if (windows->count() < 2) {
            // if count is 1, check Notebook is modified

            // if Notebook is modified, prompt to Save, Discard, or
            // Cancel

            // Handle prompt result (Cancel return; Discard proceed, no
            // save; or Save and proceed if success)
        }

        views->deleteAllIn(window);
        return true;

        // If that was last window, window service will emit the
        // lastWindowClosed signal, and App will destroy this Notebook
        // (which will automatically destroy its TempDir)
    }

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
        auto working_dir = workingDir_.path();

        if (!fnxPath_.exists()) {
            Fnx::addBlank(working_dir);
            // TODO: Mark notebook modified (maybe, maybe not until edited)?
            // (need to figure out how this will work)
        } else {
            Fnx::extract(fnxPath_, working_dir);
            // TODO: Verification (comparing Model file elements to content dir
            // files)
        }

        // Read Model.xml into memory as DOM doc
        auto dom = Fnx::makeDomDocument(working_dir);
        fnxModel_->setDomDocument(dom);

        connect(
            fnxModel_,
            &FnxModel::domChanged,
            this,
            &Notebook::onFnxModelDomChanged_);

        connect(
            fnxModel_,
            &FnxModel::elementRenamed,
            this,
            &Notebook::onFnxModelElementRenamed_);

        //...

        // TODO: Fnx control its own settings name constant?
        auto settings_file = working_dir / Constants::CONFIG_FILE_NAME;
        settings->setOverrideConfigPath(settings_file);

        registerBusCommands_();
        connectBusEvents_();
    }

    void registerBusCommands_()
    {
        bus->addCommandHandler(
            Commands::NOTEBOOK_IMPORT_FILE,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                auto dom = fnxModel_->domDocument();
                if (dom.isNull() || !workingDir_.isValid()) return;

                auto parent_dir = fnxPath_.parent();
                if (!parent_dir.exists()) return;

                auto fs_paths = Coco::PathUtil::Dialog::files(
                    cmd.context,
                    Tr::Dialogs::notebookImportFileCaption(),
                    parent_dir,
                    Tr::Dialogs::notebookImportFileFilter());

                if (fs_paths.isEmpty()) return;

                auto working_dir = workingDir_.path();
                QList<Fnx::NewFileResult> imports{};

                for (auto& fs_path : fs_paths) {
                    if (!fs_path.exists()) continue;

                    auto result =
                        Fnx::importTextFile(fs_path, working_dir, dom);
                    if (!result.isValid()) continue;

                    imports << result;
                }

                if (imports.isEmpty()) return;

                // - Ensure the model is updated before we open any files
                // - Batch insert - only writes to disk once
                QList<QDomElement> elements{};

                for (auto& i : imports)
                    if (i.isValid()) elements << i.element;

                fnxModel_->insertElements(elements, dom.documentElement());

                for (auto& i : imports) {
                    if (!i.isValid()) continue;

                    bus->execute(
                        Commands::OPEN_FILE_AT_PATH,
                        { { "path", qVar(i.path) },
                          { "title", Fnx::name(i.element) } },
                        cmd.context);
                }
            });

        // Poly commands

        bus->addCommandHandler(Commands::WS_TREE_VIEW_MODEL, [&] {
            return fnxModel_;
        });

        // TODO: Get element by tag/qualified name? (For future, when we have
        // Trash)
        bus->addCommandHandler(Commands::WS_TREE_VIEW_ROOT_INDEX, [&] {
            // The invalid index represents the root document element
            // (<notebook>). TreeView will display its children (the actual
            // files and virtual folders/structure)
            return QModelIndex{};
        });

        /// TODO: DOM & XML aren't updating...

        // Original:

        // TODO: Trigger rename immediately (maybe)
        /* bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            auto dom = fnxModel_->domDocument();
            if (dom.isNull() || !workingDir_.isValid()) return;

            auto working_dir = workingDir_.path();
            auto result = Fnx::addNewTextFile(working_dir, dom);
            if (!result.isValid()) return;

            // We append here because Fnx.h is not in charge of structure, just
            // format
            fnxModel_->insertElement(result.element, dom.documentElement());

            bus->execute(
                Commands::OPEN_FILE_AT_PATH,
                { { "path", qVar(result.path) },
                  { "title", Fnx::name(result.element) } },
                cmd.context);
        });*/

        // Ideal, maybe:

        /*bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            if (!workingDir_.isValid()) return;

            // High-level semantic operation
            auto result = fnxModel_->createAndInsertNewFile(workingDir_.path());
            if (!result.isValid()) return;

            bus->execute(
                Commands::OPEN_FILE_AT_PATH,
                { { "path", qVar(result.path) },
                  { "title", result.name } },
                cmd.context);
        });*/

        // Debug:

        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            auto dom = fnxModel_->domDocument();
            if (dom.isNull() || !workingDir_.isValid()) return;

            INFO("DOM before addNewTextFile: {}", dom.toString());

            auto working_dir = workingDir_.path();
            auto result = Fnx::addNewTextFile(working_dir, dom);
            if (!result.isValid()) return;

            INFO(
                "DOM after addNewTextFile (before insert): {}",
                dom.toString());
            INFO("Element to insert: {}", result.element.tagName());

            // We append here because Fnx.h is not in charge of structure, just
            // format
            fnxModel_->insertElement(result.element, dom.documentElement());

            INFO(
                "FnxModel's DOM after insertElement: {}",
                fnxModel_->domDocument().toString());

            bus->execute(
                Commands::OPEN_FILE_AT_PATH,
                { { "path", qVar(result.path) },
                  { "title", Fnx::name(result.element) } },
                cmd.context);
        });

        // TODO: We may not use a return value for this implementation of
        // CLOSE_TAB and CLOSE_WINDOW_TABS. We may or may not need it? If
        // we're sticking with the base class (Workspace) window close acceptor,
        // then we likely DO need it. And, too, if that's the case, then last
        // window closure may be where we handle the Notebook save prompt? So,
        // we'd do a check there if the closure is the last window...

        // Removes view without any prompt; model remains open
        // TODO: Decide on return value (see above)
        /// WIP
        bus->addCommandHandler(Commands::CLOSE_TAB, [&](const Command& cmd) {
            if (!cmd.context) return false;
            views->deleteAt(
                cmd.context,
                cmd.param<int>("index", -1)); // -1 = current
            return true;
        });

        // TODO: Decide on return value (see above)
        /// WIP
        bus->addCommandHandler(
            Commands::CLOSE_WINDOW_TABS,
            [&](const Command& cmd) {
                if (!cmd.context) return false;
                views->deleteAllIn(cmd.context);
                return true;
            });

        // Quit procedure (from Notebook's perspective):
        //...figure out after window closure

        // TODO: Should we have a "quit acceptor"? It could run a new
        // CLOSE_ALL_WINDOWS command from the base class? Allow us to handle
        // things in a specific way when the application is closing, instead of
        // just letting each window close (and possibly resulting in multiple
        // save prompts, when one would be better)?
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
    }

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
    // return bool
    void onFnxModelDomChanged_()
    {
        if (!workingDir_.isValid()) return;
        Fnx::writeModelFile(workingDir_.path(), fnxModel_->domDocument());
    }

    void onFnxModelElementRenamed_(const QDomElement& element)
    {
        if (!Fnx::isFile(element)) return;

        auto file_path = workingDir_.path() / Fnx::relativePath(element);
        auto new_name = Fnx::name(element);
        files->setPathTitleOverride(file_path, new_name);
    }

    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }

    void onTreeViewDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid() || !workingDir_.isValid()) return;

        // Notepad uses Path::isDir instead. The asymmetry bugs me, but the
        // folders here are virtual. We would still get success, since working
        // dir would be concatenated to an empty path (unless we give dirs
        // UUIDs), but it would be too abstruse

        auto element = fnxModel_->elementAt(index);
        if (Fnx::isVirtualFolder(element)) return;

        auto path = workingDir_.path() / Fnx::relativePath(element);

        bus->execute(
            Commands::OPEN_FILE_AT_PATH,
            { { "path", qVar(path) }, { "title", Fnx::name(element) } },
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

        // Temporary test action
        auto new_folder =
            menu->addAction(Tr::Menus::notebookTreeViewContextNewFolder());

        // TODO: Plan "ideal" version (like with New Tab) and pursue that, put
        // concerns in proper place, re: Fnx vs FnxMode vs Notebook
        // TODO: Trigger rename immediately (maybe)
        connect(new_folder, &QAction::triggered, this, [&, index] {
            auto dom = fnxModel_->domDocument();
            if (dom.isNull() || !workingDir_.isValid()) return;

            auto element = Fnx::addNewDir(dom);
            if (element.isNull()) return;

            // Determine where to insert it
            QDomElement parent_element{};

            if (index.isValid()) {
                // Insert as child of clicked element (whether file or folder)
                parent_element = fnxModel_->elementAt(index);
            }

            // If no valid parent found, append to root
            if (parent_element.isNull()) parent_element = dom.documentElement();

            // Model handles insertion and view update
            fnxModel_->insertElement(element, parent_element);
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
                [&, index, window] {
                    bus->execute(
                        Commands::RENAME_TREE_VIEW_INDEX,
                        { { "index", index } },
                        window);
                });
        }

        menu->popup(globalPos);
    }
};

} // namespace Fernanda
