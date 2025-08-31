#pragma once

#include <functional>

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "ColorBarModule.h"
#include "Commander.h"
#include "EventBus.h"
#include "FileService.h"
#include "MenuModule.h"
#include "SettingsModule.h"
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
        /// When using a path translator for Notebooks, which work on archives,
        /// we also need a way to take the Notepad config path as fallback
        /// without translation
    }

    // Move tracer to subclasses (Notepad and Notebook) when applicable
    virtual ~Workspace() override { COCO_TRACER; }

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
    //   // emit eventBus_->workspaceInitialized();
    // }

    void initialize(InitialWindow initialWindow = InitialWindow::No)
    {
        coreInitialization_();
        if (initialWindow) newWindow_();

        emit eventBus_->workspaceInitialized();
    }

private:
    Coco::Path root_;

    Coco::Path userDataDirectory_ = Coco::Path::Home(".fernanda");
    Coco::Path baseConfig_ = userDataDirectory_ / "Settings.ini";

    PathInterceptor pathInterceptor_ = nullptr;

    Commander* commander_ = new Commander(this);
    EventBus* eventBus_ = new EventBus(this);
    WindowService* windows_ = new WindowService(commander_, eventBus_, this);
    ViewService* views_ = new ViewService(commander_, eventBus_, this);
    FileService* files_ = new FileService(commander_, eventBus_, this);
    MenuModule* menus_ = new MenuModule(commander_, eventBus_, this);
    ColorBarModule* colorBars_ =
        new ColorBarModule(commander_, eventBus_, this);
    SettingsModule* settings_ =
        new SettingsModule(baseConfig_, commander_, eventBus_, this);

    void coreInitialization_()
    {
        /// Should this be here or App???
        Coco::PathUtil::mkdir(userDataDirectory_);

        windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);
        //...
        addCommandHandlers_();
    }

    void addCommandHandlers_();

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        return commander_->call<bool>(Calls::CloseWindowViews, {}, window);
    }

    void newWindow_()
    {
        if (auto window = windows_->make()) window->show();
    }
};

} // namespace Fernanda
