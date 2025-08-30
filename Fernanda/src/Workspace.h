#pragma once

#include <functional>

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"

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
// services and modules, and opens window creation and path filtering to the
// Application
class Workspace : public QObject
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;

    explicit Workspace(const Coco::Path& root, QObject* parent = nullptr)
        : QObject(parent)
        , root_(root)
    {
        initialize_();
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

    // This is routed through Workspace instead of handled by WindowService to
    // allow Application to open windows. This maybe could/should change in the
    // future
    void newWindow() // Args for path or session info when applicable
    {
        auto window = windows_->make();
        if (!window) return;
        window->show();
    }

private:
    Coco::Path root_;
    PathInterceptor pathInterceptor_ = nullptr;

    Commander* commander_ = new Commander(this);
    EventBus* eventBus_ = new EventBus(this);

    WindowService* windows_ = new WindowService(commander_, eventBus_, this);
    ViewService* views_ = new ViewService(commander_, eventBus_, this);
    FileService* files_ = new FileService(commander_, eventBus_, this);

    MenuModule* menus_ = new MenuModule(commander_, eventBus_, this);
    SettingsModule* settings_ = new SettingsModule(commander_, eventBus_, this);

    void initialize_();

    bool windowsCloseAcceptor_(Window* window)
    {
        if (!window) return false;
        return commander_->call<bool>(Calls::CloseWindowViews, {}, window);
    }
};

} // namespace Fernanda
