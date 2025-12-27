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
#include <QList>
#include <QSessionManager>
#include <QStringList>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AppDirs.h"
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
        setup_();
    }

    virtual ~Application() override { TRACER; }

    void initialize()
    {
        if (initialized_) return;

        Debug::initialize(Debug::Logging::Yes); // TODO: log file + toggle based
                                                // on settings or build
        if (!AppDirs::initialize()) FATAL("App directory creation failed!");

        // Eventually, get args + session info
        // auto args = arguments();
        // Make session objects
        initializeNotepad_();
        // Any Notebooks needed via Session

        initialized_ = true;

        /// Test:
        // makeNotebook_(AppDirs::defaultDocs() / "Unsaved.fnx");
        // makeNotebook_(AppDirs::defaultDocs() / "Saved.fnx");

        /*auto test_1 = "X:/Test/Path.file"_ccpath;
        auto test_2 = "Y:/Drive"_ccpath;
        auto test_3 = "Part"_ccpath;

        qDebug() << test_1;
        qDebug() << test_2;
        qDebug() << test_3;

        qDebug() << test_2 / test_3;*/
    }

public slots:
    void onStartCopAppRelaunched(const QStringList& args)
    {
        //...
    }

    void tryQuit()
    {
        // TODO: Go by Z-order (most to least recently used). Currently, we go
        // by most to least recently opened
        for (auto i = notebooks_.count() - 1; i >= 0; --i)
            if (!notebooks_.at(i)->tryQuit()) return;

        if (notepad_ && !notepad_->tryQuit()) return;

        quit();
    }

private:
    bool initialized_ = false;
    Notepad* notepad_ = nullptr;
    QList<Notebook*> notebooks_{};

    void setup_()
    {
        setOrganizationName(VERSION_AUTHOR_STRING);
        setOrganizationDomain(VERSION_DOMAIN);
        setApplicationName(VERSION_APP_NAME_STRING);
        setApplicationVersion(VERSION_FULL_STRING);
        setQuitOnLastWindowClosed(false);

        connect(
            this,
            &Application::commitDataRequest,
            this,
            &Application::onCommitDataRequest_);
    }

    void initializeNotepad_()
    {
        // Temporary opening procedures:
        notepad_ = new Notepad(this);

        connect(notepad_, &Notepad::lastWindowClosed, this, [&] {
            if (notebooks_.isEmpty()) quit();
        });

        connectWorkspace_(notepad_);

        // Will only open new window if: 1) there is no Notebook from sessions;
        // 2) there is no Notepad window from sessions
        // TODO: Move this to initialize()?
        notepad_->open(NewWindow::Yes);
    }

    void makeNotebook_(const Coco::Path& fnxPath)
    {
        auto notebook = new Notebook(fnxPath, this);
        notebooks_ << notebook;

        connect(notebook, &Notebook::lastWindowClosed, this, [&, notebook] {
            notebooks_.removeAll(notebook);
            delete notebook;
            if (notebooks_.isEmpty() && !notepad_->hasWindows()) quit();
        });

        connect(notebook, &Notebook::openNotepadRequested, this, [&] {
            notepad_->hasWindows() ? notepad_->activate()
                                   : notepad_->newWindow();
        });

        connectWorkspace_(notebook);

        notebook->open(NewWindow::Yes);
    }

    // For joint Workspace connections
    void connectWorkspace_(Workspace* workspace)
    {
        if (!workspace) return;

        connect(
            workspace,
            &Workspace::newNotebookRequested,
            this,
            [&](const Coco::Path& fnxPath) { makeNotebook_(fnxPath); });

        connect(
            workspace,
            &Workspace::openNotebookRequested,
            this,
            [&](const Coco::Path& fnxPath) {
                // Shouldn't need to check Fnx::isFnxFile. The promise of this
                // signal is "open Notebook" not "open maybe a Notebook"!
                for (auto& notebook : notebooks_) {
                    if (notebook->fnxPath() == fnxPath) {
                        notebook->activate();
                        return;
                    }
                }

                makeNotebook_(fnxPath);
            });
    }

private slots:
    // TODO: Needs tested!
    void onCommitDataRequest_(QSessionManager& manager)
    {
        // TODO: Go by Z-order (most to least recently used). Currently, we go
        // by most to least recently opened
        for (auto i = notebooks_.count() - 1; i >= 0; --i) {
            if (!notebooks_.at(i)->tryQuit()) {
                manager.cancel();
                return;
            }
        }

        if (notepad_ && !notepad_->tryQuit()) {
            manager.cancel();
            return;
        }
    }
};

inline Application* app()
{
    return static_cast<Application*>(QCoreApplication::instance());
}

} // namespace Fernanda
