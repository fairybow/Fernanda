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

#include <QAbstractItemModel>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Bus.h"
#include "Constants.h"
#include "NotepadMenuModule.h"
#include "Utility.h"
#include "Version.h"
#include "Workspace.h"

namespace Fernanda {

// A Workspace that operates on the OS filesystem. There is only 1 Notepad
// during the application lifetime
class Notepad : public Workspace
{
    Q_OBJECT

public:
    using PathInterceptor = std::function<bool(const Coco::Path&)>;

    Notepad(const Coco::Path& globalConfig, QObject* parent = nullptr)
        : Workspace(globalConfig, parent)
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
    Coco::Path currentBaseDir_ = Coco::Path::Documents(VERSION_APP_NAME_STRING);

    PathInterceptor pathInterceptor_ = nullptr;
    NotepadMenuModule* menus_ = new NotepadMenuModule(bus, this);

    void initialize_()
    {
        bus->addInterceptor(Cmd::OpenFile, [&](const Command& cmd) {
            if (pathInterceptor_
                && pathInterceptor_(to<QString>(cmd.params, "path"))) {
                return true;
            }

            return false;
        });

        bus->addCommandHandler(Cmd::NotepadBaseDir, [&] {
            return currentBaseDir_.toQString();
        });
    }

    virtual QAbstractItemModel* makeTreeViewModel_() override
    {
        auto model = new QFileSystemModel(this);
        auto root_index = model->setRootPath(currentBaseDir_.toQString());
        storeItemModelRootIndex(model, root_index);
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        // Any other Notepad-specific model setup
        return model;
    }
};

} // namespace Fernanda
