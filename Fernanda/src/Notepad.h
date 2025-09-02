#pragma once

#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "MenuModule.h"
#include "SettingsModule.h"
#include "Workspace.h"

namespace Fernanda {

// A Workspace that operates on the OS filesystem. There is only 1 Notepad
// during the application lifetime
class Notepad : public Workspace
{
    Q_OBJECT

public:
    explicit Notepad(
        const Coco::Path& config, // could move config/base to workspace
        const Coco::Path& root,
        QObject* parent = nullptr)
        : Workspace(root, parent)
        , config_(config)
    {
        initialize_();
    }

    virtual ~Notepad() override { COCO_TRACER; }

private:
    Coco::Path config_;

    MenuModule* menus_ = new MenuModule(commander, eventBus, this);
    SettingsModule* settings_ = nullptr;

    void initialize_()
    {
        settings_ = new SettingsModule(config_, commander, eventBus, this);
    }
};

} // namespace Fernanda
