#pragma once

#include <QFont>
#include <QObject>
#include <QString>

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

    void settingEditorFontChanged(const QFont& font);

    // Maybe:

    // void workspaceInitialized(Workspace* workspace);
    // void workspaceShuttingDown(Workspace* workspace);
    // void windowShown(Window* window);
    // void windowClosed(Window* window);
    // void lastWindowClosed(); (in WinService, uncomment later)
    // void settingsDialogRequested(Window* window);

private:
    void initialize_();
};

} // namespace Fernanda
