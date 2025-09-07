/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractItemModel>
#include <QLabel>
#include <QObject>
#include <QStandardItemModel> /// Temp
#include <QStatusBar>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "NotebookMenuModule.h"
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
        const Coco::Path& archivePath,
        const Coco::Path& globalConfig,
        const Coco::Path& userDataDir,
        QObject* parent = nullptr)
        : Workspace(globalConfig, parent)
        , archivePath_(archivePath)
        , userDataDir_(userDataDir)
    {
        initialize_();
    }

    virtual ~Notebook() override { COCO_TRACER; }

    Coco::Path archivePath() const noexcept { return archivePath_; }
    // Coco::Path root() const noexcept { return root_; } // Probably
    // internal-only

private:
    Coco::Path archivePath_;
    Coco::Path userDataDir_;

    QString name_{};
    Coco::Path root_{};
    Coco::Path content_{};

    NotebookMenuModule* menus_ =
        new NotebookMenuModule(commander, eventBus, this);

    void initialize_()
    {
        name_ = archivePath_.stemQString();

        // 1. Extract

        // 2. Set root

        // 3. Set settings override
        // settings->setOverrideConfigPath(root / Settings.ini);

        commander->addQueryHandler(Queries::NotebookRoot, [&] {
            return root_.toQString();
        });

        connect(
            eventBus,
            &EventBus::windowCreated,
            this,
            &Notebook::onWindowCreated_);
    }

    virtual QAbstractItemModel* makeTreeViewModel_() override
    {
        // TODO: Replace with ArchiveModel when implemented
        auto model = new QStandardItemModel(this);
        // Configure archive-specific settings
        return model;
    }

    void addWorkspaceIndicator_(Window* window)
    {
        if (!window) return;

        auto status_bar = window->statusBar();
        if (!status_bar) return; // <- Shouldn't happen
        auto temp_label = new QLabel;
        temp_label->setText(name_);
        status_bar->addPermanentWidget(temp_label);
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addWorkspaceIndicator_(window);
    }
};

} // namespace Fernanda
