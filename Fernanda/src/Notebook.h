#pragma once

#include <QLabel>
#include <QObject>
#include <QStatusBar>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "SettingsModule.h"
#include "Window.h"
#include "Workspace.h"

namespace Fernanda {

// A binder-style Workspace that operates on a 7zip archive-based filesystem.
// There can be any number of Notebooks open during the application lifetime
class Notebook : public Workspace
{
    Q_OBJECT

public:
    Notebook(
        const Coco::Path& baseConfig,
        const Coco::Path& overridingConfig,
        const Coco::Path& root,
        QObject* parent = nullptr)
        : Workspace(baseConfig, root, parent)
        , overridingConfig_(overridingConfig) // Archive path
    {
        initialize_();
    }

    virtual ~Notebook() override { COCO_TRACER; }

private:
    Coco::Path overridingConfig_;

    void initialize_()
    {
        settings->setOverrideConfigPath(overridingConfig_);

        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            auto status_bar = window->statusBar();
            if (!status_bar) return; // <- Shouldn't happen
            auto temp_label = new QLabel;
            temp_label->setText("[Archive Name]");
            status_bar->addPermanentWidget(temp_label);
        });
    }
};

} // namespace Fernanda
