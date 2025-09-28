/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QApplication>
#include <QStringList>
#include <QSet>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Constants.h"
#include "Debug.h"
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

    virtual ~Application() override { TRACER; }

    void initialize()
    {
        setOrganizationName(VERSION_AUTHOR_STRING);
        setOrganizationDomain(VERSION_DOMAIN);
        setApplicationName(VERSION_APP_NAME_STRING);
        setApplicationVersion(VERSION_FULL_STRING);
        setQuitOnLastWindowClosed(false);

        Debug::initialize(true); // file later
        Coco::PathUtil::mkdir(userDataDirectory_);

        // auto args = arguments();

        // Make session objects

        initializeNotepad_();
        // Any Notebooks needed via Session
    }

public slots:
    void onRelaunchAttempted(QStringList args)
    {
        //...
    }

private:
    Coco::Path userDataDirectory_ = Coco::Path::Home(".fernanda");
    Coco::Path globalConfig_ = userDataDirectory_ / CONFIG_FILE_NAME;

    Notepad* notepad_ = nullptr;
    QSet<Notebook*> notebooks_{};

    void initializeNotepad_()
    {
        // Temporary opening procedures:
        notepad_ = new Notepad(globalConfig_, this);
        notepad_->setPathInterceptor(
            this,
            &Application::notepadPathInterceptor_);

        // Will only open new window if: 1) there is no Notebook from sessions;
        // 2) there is no Notepad window from sessions
        notepad_->open(NewWindow::Yes);
    }

    void makeNotebook_(const Coco::Path& archive)
    {
        // Temporary opening procedures:
        auto notebook =
            new Notebook(archive, globalConfig_, userDataDirectory_, this);
        notebooks_ << notebook;

        connect(notebook, &Notebook::lastWindowClosed, this, [=] {
            // Clean-up
            notebooks_.remove(notebook);
            delete notebook;
        });

        notebook->open(NewWindow::Yes);
    }

    bool notepadPathInterceptor_(const Coco::Path& path)
    {
        if (FileTypes::is(FileTypes::SevenZip, path)) {
            INFO("Interceptor true.");

            for (auto& notebook : notebooks_) {
                if (notebook->archivePath() == path) {
                    notebook->activate();
                    return true;
                }
            }

            makeNotebook_(path);
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
