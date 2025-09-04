#pragma once

#include <QLabel>
#include <QObject>
#include <QStandardItemModel> /// Temp
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
        const Coco::Path& root,
        QObject* parent = nullptr)
        : Workspace(baseConfig, root, parent)
    {
        initialize_();
    }

    virtual ~Notebook() override { COCO_TRACER; }

private:
    void initialize_()
    {
        // Set this after extraction
        //settings->setOverrideConfigPath(root / Settings.ini);

        connect(eventBus, &EventBus::windowCreated, this, [&](Window* window) {
            auto status_bar = window->statusBar();
            if (!status_bar) return; // <- Shouldn't happen
            auto temp_label = new QLabel;
            temp_label->setText("[Archive Name]");
            status_bar->addPermanentWidget(temp_label);
        });
    }

    virtual QAbstractItemModel* makeTreeViewModel_() override
    {
        // TODO: Replace with ArchiveModel when implemented
        auto model = new QStandardItemModel(this);
        // Configure archive-specific settings
        return model;
    }
};

} // namespace Fernanda
