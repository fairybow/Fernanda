#pragma once

#include <QApplication>
#include <QStringList>

#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"

#include "FileTypes.h"
#include "Version.h"
#include "Workspace.h" // Replace with subclasses when applicable

namespace Fernanda {

// Top-level application coordinator and entry point that creates and manages
// Workspaces and handles application lifecycle
//
// Note: When no paths or previous sessions (future) are provided to Fernanda on
// opening, it will open a blank Notepad; otherwise, it will open the correct
// workspaces for the paths, Notepad and/or Notebook(s)
class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv)
        : QApplication(argc, argv)
    {
    }

    virtual ~Application() override { COCO_TRACER; }

    void initialize()
    {
        setOrganizationName(VERSION_AUTHOR_STRING);
        setOrganizationDomain(VERSION_DOMAIN);
        setApplicationName(VERSION_APP_NAME_STRING);
        setApplicationVersion(VERSION_FULL_STRING);

        Coco::PathUtil::mkdir(userDataDirectory_);
        Coco::PathUtil::mkdir(notepadRoot_);

        // auto args = arguments();

        // Temporary opening procedures:
        notepad_ = new Workspace(notepadRoot_, this);
        notepad_->setPathInterceptor(
            this,
            &Application::notepadPathInterceptor_);
        notepad_->initialize(Workspace::InitialWindow::Yes);
    }

public slots:
    void onRelaunchAttempted(QStringList args)
    {
        //...
    }

private:
    Coco::Path userDataDirectory_ = Coco::Path::Home(".fernanda");
    Coco::Path notepadRoot_ = Coco::Path::Documents("Fernanda");

    Workspace* notepad_ = nullptr; // Replace with subclass when applicable
    // TestNotebook* testNotebook_ = nullptr;
    // QList<Notebook*> notebooks_{};

    bool notepadPathInterceptor_(const Coco::Path& path)
    {
        if (FileTypes::is(FileTypes::SevenZip, path)) {
            // For now, just don't open 7zips
            COCO_LOG_THIS("Interceptor true.");
            return true;
        }

        return false;
    }
};

inline Application* app()
{
    return static_cast<Application*>(QCoreApplication::instance());
}

} // namespace Fernanda
