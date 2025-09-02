#pragma once

#include <QObject>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "SettingsModule.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace that operates on a 7zip archive-based filesystem.
// There can be any number of Notebooks open during the application lifetime
class Notebook : public Workspace
{
    Q_OBJECT

public:
    explicit Notebook(
        const Coco::Path& baseConfig,
        const Coco::Path& overridingConfig,
        const Coco::Path& root,
        QObject* parent = nullptr)
        : Workspace(root, parent)
        , baseConfig_(baseConfig) // Normal path
        , overridingConfig_(overridingConfig) // Archive path
    {
        initialize_();
    }

    virtual ~Notebook() override { COCO_TRACER; }

private:
    Coco::Path baseConfig_;
    Coco::Path overridingConfig_;

    SettingsModule* settings_ = nullptr;

    void initialize_()
    {
        settings_ = new SettingsModule(baseConfig_, commander, eventBus, this);
        settings_->setOverrideConfigPath(overridingConfig_);
    }
};

} // namespace Fernanda
