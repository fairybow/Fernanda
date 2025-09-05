/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVariant>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "IFileModel.h"

namespace Fernanda {

class IFileView;
class Window;

// Centralized notifications for use by the Workspace's Services and Modules
class EventBus : public QObject
{
    Q_OBJECT

public:
    explicit EventBus(QObject* parent = nullptr)
        : QObject(parent)
    {
        initialize_();
    }

    virtual ~EventBus() override { COCO_TRACER; }

signals:
    // Workspace

    void workspaceInitialized();

    // WindowService

    void windowCreated(Window* window);
    void visibleWindowCountChanged(int count);
    void lastWindowClosed();

    // Window may be nullptr!
    void activeWindowChanged(Window* window);
    void windowDestroyed(Window* window);

    // FileService

    void fileReadied(IFileModel* model, Window* window);
    void fileModificationChanged(IFileModel* model, bool modified);
    void fileMetaChanged(IFileModel* model);
    void fileSaved(SaveResult result, const Coco::Path& path);
    void fileSavedAs(
        SaveResult result,
        const Coco::Path& path,
        const Coco::Path& oldPath = {});
    void windowSaveExecuted(Window* window, SaveResult result);
    void workspaceSaveExecuted(SaveResult result);

    // ViewService

    void windowTabCountChanged(Window* window, int count);

    // View may be nullptr!
    void activeFileViewChanged(IFileView* view, Window* window);
    void viewClosed(IFileView* view);

    // SettingsModule

    void settingChanged(const QString& key, const QVariant& value);

    // Maybe:

    // void workspaceShuttingDown(Workspace* workspace);
    // void windowShown(Window* window);
    // void windowClosed(Window* window);

private:
    void initialize_();
};

} // namespace Fernanda
