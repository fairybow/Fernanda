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
#include <QLabel>
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

        //...

        // TODO: Fnx control its own settings name constant?
        auto settings_file = working_dir / Constants::CONFIG_FILE_NAME;
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

        // TODO: Get element by tag/qualified name? (For future, when we have
        // Trash)
        bus->addCommandHandler(Commands::TREE_VIEW_ROOT_INDEX, [&] {
            // The invalid index represents the root document element
            // (<notebook>). TreeView will display its children (the actual
            // files and virtual folders/structure)
            return QModelIndex{};
        });

        // TODO: Trigger rename immediately, when that functionality is
        // available
        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            if (!cmd.context) return;
            auto dom = fnxModel_->domDocument();
            if (dom.isNull() || !workingDir_.isValid()) return;

            auto working_dir = workingDir_.path();
            auto result = Fnx::addNewTextFile(working_dir, dom);
            if (!result.isValid()) return;

            // We append here because Fnx.h is not in charge of structure, just
            // format
            fnxModel_->insertElement(result.element, dom.documentElement());
            Fnx::writeModelFile(working_dir, dom);

            bus->execute(
                Commands::OPEN_FILE_AT_PATH,
                { { "path", qVar(result.path) },
                  { "title", Fnx::name(result.element) } },
                cmd.context);
        });

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

                // Ensure the model is updated before we open any files
                for (auto& i : imports) {
                    if (!i.isValid()) continue;
                    fnxModel_->insertElement(i.element, dom.documentElement());
                }

                Fnx::writeModelFile(working_dir, dom);

                for (auto& i : imports) {
                    if (!i.isValid()) continue;

                    bus->execute(
                        Commands::OPEN_FILE_AT_PATH,
                        { { "path", qVar(i.path) },
                          { "title", Fnx::name(i.element) } },
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
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }

    void onTreeViewDoubleClicked_(Window* window, const QModelIndex& index)
    {
        if (!window || !index.isValid()) return;

        // Notepad uses Path::isDir instead. The asymmetry bugs me, but the
        // folders here are virtual. We would still get success, since working
        // dir would be concatenated to an empty path (unless we give dirs
        // UUIDs), but it would be too abstruse

        auto element = fnxModel_->elementAt(index);
        if (Fnx::isDir(element)) return;

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

        // TODO: Trigger rename immediately
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

            // Persist to disk
            Fnx::writeModelFile(workingDir_.path(), dom);
        });

        menu->popup(globalPos);
    }
};

} // namespace Fernanda
