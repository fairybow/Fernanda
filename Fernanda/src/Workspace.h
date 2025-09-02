#pragma once

#include <functional>

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
#include "ViewService.h"
#include "Window.h"
#include "WindowService.h"

namespace Fernanda {

// Base class for Notepad and Notebook workspaces (collection of windows, their
// files, and the filesystems on which they operate). Owns and initializes
// services and modules, and allows path filtering for the Application
class Workspace : public QObject
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;
    COCO_BOOL(InitialWindow);

    explicit Workspace(const Coco::Path& root, QObject* parent = nullptr)
        : QObject(parent)
        , root_(root)
    {
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

    PathInterceptor pathInterceptor() const noexcept
    {
        return pathInterceptor_;
    }
    void setPathInterceptor(const PathInterceptor& pathInterceptor)
    {
        pathInterceptor_ = pathInterceptor;
    }

    template <typename ClassT>
    void setPathInterceptor(
        ClassT* object,
        bool (ClassT::*method)(const Coco::Path&))
    {
        pathInterceptor_ = [object, method](const Coco::Path& path) {
            return (object->*method)(path);
        };
    }

    // void initialize(const Session& session)
    // {
    //   // coreInitialization_();
    //   // ...open Session...
    //   // emit eventBus->workspaceInitialized();
    // }

    void initialize(InitialWindow initialWindow = InitialWindow::No)
    {
        coreInitialization_();
        if (initialWindow) newWindow_();

        emit eventBus->workspaceInitialized();
    }

protected:
    Commander* commander = new Commander(this);
    EventBus* eventBus = new EventBus(this);

private:
    Coco::Path root_;

    PathInterceptor pathInterceptor_ = nullptr;
    WindowService* windows_ = new WindowService(commander, eventBus, this);
    ViewService* views_ = new ViewService(commander, eventBus, this);
    FileService* files_ = new FileService(commander, eventBus, this);
    ColorBarModule* colorBars_ = new ColorBarModule(commander, eventBus, this);

    void coreInitialization_()
    {
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
};

} // namespace Fernanda
