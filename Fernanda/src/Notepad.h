#pragma once

#include <functional>

#include <QFileSystemModel>
#include <QModelIndex>
#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "MenuModule.h"
#include "Utility.h"
#include "Workspace.h"

namespace Fernanda {

// A Workspace that operates on the OS filesystem. There is only 1 Notepad
// during the application lifetime
class Notepad : public Workspace
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;

    Notepad(
        const Coco::Path& configPath,
        const Coco::Path& rootPath,
        QObject* parent = nullptr)
        : Workspace(configPath, rootPath, parent)
    {
        initialize_();
    }

    virtual ~Notepad() override { COCO_TRACER; }

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

private:
    PathInterceptor pathInterceptor_ = nullptr;
    MenuModule* menus_ = new MenuModule(commander, eventBus, this);

    void initialize_()
    {
        commander->addInterceptor(Commands::OpenFile, [&](Command& cmd) {
            if (pathInterceptor_
                && pathInterceptor_(to<QString>(cmd.params, "path"))) {
                return true;
            }

            return false;
        });
    }

    virtual QAbstractItemModel* makeTreeViewModel_() override
    {
        auto model = new QFileSystemModel(this);
        auto root_index = model->setRootPath(root.toQString());
        storeItemModelRootIndex(model, root_index);
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        // Any other Notepad-specific model setup
        return model;
    }
};

} // namespace Fernanda
