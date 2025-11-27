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
#include <QSet>
#include <QStringList>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AppDirs.h"
#include "Constants.h"
#include "Debug.h"
#include "FileTypes.h"
#include "Notebook.h"
#include "Notepad.h"
#include "Version.h"

namespace Fernanda {

// Top-level application coordinator and entry point that creates and manages
// Workspaces and handles application lifecycle
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
        if (initialized_) return;

        setProperties_();
        setup_();
        // Eventually, get args + session info
        // auto args = arguments();
        // Make session objects
        initializeNotepad_();
        // Any Notebooks needed via Session

        initialized_ = true;

        /// Test:
        // makeNotebook_(AppDirs::defaultDocs() / "Unsaved.fnx");
        makeNotebook_(AppDirs::defaultDocs() / "Saved.fnx");

        /*auto test_1 = "X:/Test/Path.file"_ccpath;
        auto test_2 = "Y:/Drive"_ccpath;
        auto test_3 = "Part"_ccpath;

        qDebug() << test_1;
        qDebug() << test_2;
        qDebug() << test_3;

        qDebug() << test_2 / test_3;*/
    }

public slots:
    // TODO: const ref args?
    void onRelaunchAttempted(QStringList args)
    {
        //...
    }

    void tryQuit()
    {
        for (auto& notebook : notebooks_)
            if (!notebook->canQuit()) return;

        if (!notepad_->canQuit()) return;

        quit();
    }

private:
    bool initialized_ = false;
    Notepad* notepad_ = nullptr;
    QSet<Notebook*> notebooks_{};

    void setProperties_()
    {
        setOrganizationName(VERSION_AUTHOR_STRING);
        setOrganizationDomain(VERSION_DOMAIN);
        setApplicationName(VERSION_APP_NAME_STRING);
        setApplicationVersion(VERSION_FULL_STRING);
        setQuitOnLastWindowClosed(false);
    }

    void setup_() const
    {
        Debug::initialize(Debug::Logging::Yes); // Add file later (in user data)
        if (!AppDirs::initialize()) FATAL("App directory creation failed!");
    }

    void initializeNotepad_()
    {
        // Temporary opening procedures:
        notepad_ = new Notepad(this);
        notepad_->setPathInterceptor(
            this,
            &Application::notepadPathInterceptor_);

        // Will only open new window if: 1) there is no Notebook from sessions;
        // 2) there is no Notepad window from sessions
        // TODO: Move this to initialize()?
        notepad_->open(NewWindow::Yes);
    }

    void makeNotebook_(const Coco::Path& fnx)
    {
        // Temporary opening procedures:
        auto notebook = new Notebook(fnx, this);
        notebooks_ << notebook;

        // TODO: We'll have to move this probably, since we'll need to
        // potentially quit, too, if this is the last Notebook and no Notepad
        // windows are open
        connect(notebook, &Notebook::lastWindowClosed, this, [&, notebook] {
            // Clean-up
            notebooks_.remove(notebook);
            delete notebook;
        });

        notebook->open(NewWindow::Yes);
    }

    // TODO: Ensure .fnx extension, too?
    bool notepadPathInterceptor_(const Coco::Path& path)
    {
        if (FileTypes::is(FileTypes::SevenZip, path)) {
            INFO("Notepad intercepted {}", path);

            for (auto& notebook : notebooks_) {
                if (notebook->fnxPath() == path) {
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
