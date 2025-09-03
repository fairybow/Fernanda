#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "ColorBarModule.h"
#include "Commander.h"
#include "EventBus.h"
#include "FileService.h"
#include "SettingsModule.h"
#include "TreeViewModule.h"
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

COCO_BOOL(NewWindow);

// Base class for Notepad and Notebook workspaces (collection of windows, their
// files, and the filesystems on which they operate). Owns and initializes
// services and modules, and allows path filtering for the Application
class Workspace : public QObject
{
    Q_OBJECT

public:
    Workspace(
        const Coco::Path& configPath,
        const Coco::Path& rootPath,
        QObject* parent = nullptr)
        : QObject(parent)
        , config(configPath)
        , root_(rootPath)
    {
        initialize_();

        /// Unsure about passing config paths via App. Notebooks will know their
        /// config path is `archive/settings.ini`, why pass that via App? May
        /// instead want this approach: all Workspaces have a base config path
        /// (this will act as a fallback in Notebooks and primary in Notepad).
        /// We can have a protected method to add a new (overriding) config path
        ///
        /// Similar can be said of root...
        ///
        /// Additionally, root means, essentially, the starting directory for
        /// opening files. For Notepad, this is your typical Documents/App
        /// folder. For Notebooks, this is the archive root. The plan is that
        /// Notepad's root will eventually be mutable and saved via config or
        /// session, whereas Notebook's will be static.
        ///
        /// When using a path translator for Notebooks, which work on archives,
        /// we also need a way to take the Notepad config path as fallback
        /// without translation
        ///
        /// This is an option but may be overkill: an app-wide communication
        /// strategy, like Commander/EventBus, but for Workspaces and App...
    }

    virtual ~Workspace() override = default;

    // void open(const Session& session)
    // {
    //   // ...open Session...
    //   // emit eventBus->workspaceInitialized();
    // }

    void open(NewWindow withWindow = NewWindow::No)
    {
        if (withWindow) newWindow_();
        emit eventBus->workspaceInitialized();
    }

protected:
    Coco::Path config;

    Commander* commander = new Commander(this);
    EventBus* eventBus = new EventBus(this);

    SettingsModule* settings = nullptr;

private:
    Coco::Path root_; // Maybe protected later

    WindowService* windows_ = new WindowService(commander, eventBus, this);
    ViewService* views_ = new ViewService(commander, eventBus, this);
    FileService* files_ = new FileService(commander, eventBus, this);
    TreeViewModule* treeViews_ = new TreeViewModule(commander, eventBus, this);
    ColorBarModule* colorBars_ = new ColorBarModule(commander, eventBus, this);

    void initialize_()
    {
        settings = new SettingsModule(config, commander, eventBus, this);
        windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);
        //...
        addCommandHandlers_();
    }

    void addCommandHandlers_();

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        return commander->call<bool>(Calls::CloseWindowViews, {}, window);
    }

    void newWindow_()
    {
        if (auto window = windows_->make()) window->show();
    }

    virtual QAbstractItemModel* makeTreeViewModel_() = 0;
};

} // namespace Fernanda
