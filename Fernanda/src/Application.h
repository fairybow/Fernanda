#pragma once

#include <QApplication>
#include <QStringList>

#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "FileTypes.h"
#include "Notebook.h"
#include "Notepad.h"
#include "Version.h"

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

        // auto args = arguments();

        // Make session objects

        initializeNotepad_();
        // Notebooks...
        initializeTestNotebook_();
    }

public slots:
    void onRelaunchAttempted(QStringList args)
    {
        //...
    }

private:
    Coco::Path userDataDirectory_ = Coco::Path::Home(".fernanda");
    Coco::Path notepadConfig_ = userDataDirectory_ / "Settings.ini";
    Coco::Path notepadRoot_ = Coco::Path::Documents("Fernanda");

    Notepad* notepad_ = nullptr; // Replace with subclass when applicable
    Notebook* testNotebook_ = nullptr; /// Remove later
    // QList<Notebook*> notebooks_{};

    void initializeNotepad_()
    {
        Coco::PathUtil::mkdir(notepadRoot_);

        // Temporary opening procedures:
        notepad_ = new Notepad(notepadConfig_, notepadRoot_, this);
        notepad_->setPathInterceptor(
            this,
            &Application::notepadPathInterceptor_);

        // Will only open new window if: 1) there is no Notebook from sessions;
        // 2) there is no Notepad window from sessions
        notepad_->open(Workspace::InitialWindow::Yes);
    }

    void initializeTestNotebook_()
    {
        // Temporary opening procedures:
        testNotebook_ = new Notebook(
            notepadConfig_,
            userDataDirectory_ / "TestNotebookSettings.ini",
            {}, /// Fine for now
            this);

        testNotebook_->open(Workspace::InitialWindow::Yes);
    }

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
