/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QApplication>
#include <QFontDatabase>
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
        // TODO: Session handling

        if (initialized_) return;

        Debug::initialize(); // TODO: Log file path
        if (!AppDirs::initialize()) FATAL("App directory creation failed!");
        loadBundledFonts_();

        initializeNotepad_();

        auto parsed_args = parseArgs_(arguments());

        if (parsed_args.isEmpty()) {
            // Open Notepad single empty window, show color bar pastel
            notepad_->show();
            notepad_->beCute();

        } else if (parsed_args.hasOnlyFnx()) {
            // Make a Notebook for each FNX file, open single window for each,
            // show color bar pastel in each Notebook's window
            for (auto& path : parsed_args.fnxFiles) {
                if (auto notebook = makeNotebook_(path)) {
                    notebook->show();
                    notebook->beCute();
                }
            }
        } else if (parsed_args.hasOnlyRegular()) {
            // Open Notepad single window with a tab for each file, show color
            // bar pastel
            notepad_->show();
            // Might not be working:
            notepad_->openFiles(parsed_args.regularFiles);
            notepad_->beCute();
        } else {
            // Has both:
            // - Open Notepad single window with a tab for each file, show color
            // bar pastel
            // - Make a Notebook for each FNX file, open single window for each,
            // show color bar pastel in each Notebook's window
            notepad_->show();
            notepad_->openFiles(parsed_args.regularFiles);
            notepad_->beCute();

            for (auto& path : parsed_args.fnxFiles) {
                if (auto notebook = makeNotebook_(path)) {
                    notebook->show();
                    notebook->beCute();
                }
            }
        }

        initialized_ = true;
    }

public slots:
    void onStartCopAppRelaunched(const QStringList& args)
    {
        // - These args are received via trying to reopen Fernanda. Just
        // clicking the EXE will send only the executable path (arg 0).
        // - We'll receive new args by clicking associated files (usually just
        // .fnx but could be anything if user sets it in their OS)
        // - If we receive Notebook files, open the Notebooks for those
        // - If we receive any regular files, open those in the top most Notepad
        // window

        auto parsed_args = parseArgs_(args);

        if (parsed_args.isEmpty()) {
            // Activate Notepad (if open) and each Notebook, to raise them all
            notepad_->activate(); // No-op if no windows

            for (auto& notebook : notebooks_)
                notebook->activate();

        } else if (parsed_args.hasOnlyFnx()) {
            // - Make a Notebook for each FNX file, open single window for each,
            // show color bar pastel in each Notebook's window
            // - If any of the Notebook files is already open, activate it
            // instead
            for (auto& path : parsed_args.fnxFiles) {
                bool found = false;

                for (auto& notebook : notebooks_) {
                    if (notebook->fnxPath() == path) {
                        notebook->activate();
                        found = true;
                        break;
                    }
                }

                if (found) continue;

                if (auto notebook = makeNotebook_(path)) {
                    notebook->show();
                    notebook->beCute();
                }
            }

        } else if (parsed_args.hasOnlyRegular()) {
            // - If Notepad has no windows, open Notepad single window with a
            // tab for each file (maybe do not show color bar pastel - would
            // have to use an initialize function?)
            // - If Notepad has windows, open a tab for each file in the
            // top-most window
            notepad_->show();
            notepad_->openFiles(parsed_args.regularFiles);

        } else {
            // Has both:
            // - If Notepad has no windows, open Notepad single window with a
            // tab for each file (maybe do not show color bar pastel - would
            // have to use an initialize function?)
            // - If Notepad has windows, open a tab for each file in the
            // top-most window
            // - Make a Notebook for each FNX file, open single window for each,
            // show color bar pastel in each Notebook's window
            // - If any of the Notebook files is already open, activate it
            // instead
            notepad_->show();
            notepad_->openFiles(parsed_args.regularFiles);

            for (auto& path : parsed_args.fnxFiles) {
                bool found = false;

                for (auto& notebook : notebooks_) {
                    if (notebook->fnxPath() == path) {
                        notebook->activate();
                        found = true;
                        break;
                    }
                }

                if (found) continue;

                if (auto notebook = makeNotebook_(path)) {
                    notebook->show();
                    notebook->beCute();
                }
            }
        }
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
        // NB: Logging will not work in this function! (It's initialized in
        // Application::initialize, called after construction)

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

    void loadBundledFonts_()
    {
        for (auto& path : { ":/mononoki/mononoki-Regular.otf",
                            ":/mononoki/mononoki-Bold.otf",
                            ":/mononoki/mononoki-Italic.otf",
                            ":/mononoki/mononoki-BoldItalic.otf" }) {
            if (QFontDatabase::addApplicationFont(path) < 0)
                WARN("Failed to load font: {}", path);
        }
    }

    void initializeNotepad_()
    {
        notepad_ = new Notepad(this);

        connectWorkspace_(notepad_);

        connect(notepad_, &Notepad::lastWindowClosed, this, [&] {
            if (notebooks_.isEmpty()) quit();
        });
    }

    Notebook* makeNotebook_(const Coco::Path& fnxPath)
    {
        auto notebook = new Notebook(fnxPath, this);
        notebooks_ << notebook;

        connectWorkspace_(notebook);

        connect(notebook, &Notebook::lastWindowClosed, this, [&, notebook] {
            notebooks_.removeAll(notebook);
            delete notebook;
            if (notebooks_.isEmpty() && !notepad_->hasWindows()) quit();
        });

        connect(notebook, &Notebook::openNotepadRequested, this, [&] {
            notepad_->show();
        });

        return notebook;
    }

    // For common Workspace connections
    void connectWorkspace_(Workspace* workspace)
    {
        if (!workspace) return;

        connect(
            workspace,
            &Workspace::newNotebookRequested,
            this,
            [&](const Coco::Path& fnxPath) {
                if (auto notebook = makeNotebook_(fnxPath)) {
                    notebook->show();
                    notebook->beCute();
                }
            });

        connect(
            workspace,
            &Workspace::openNotebookRequested,
            this,
            [&](const Coco::Path& fnxPath) {
                // Shouldn't need to check Fnx::isFnxFile. The promise of this
                // signal is "open Notebook" not "open maybe a Notebook"!
                // TODO: Although, we may need to do some redesign if we want to
                // prompt for files that are .fnx by extension only...
                for (auto& notebook : notebooks_) {
                    if (notebook->fnxPath() == fnxPath) {
                        notebook->activate();
                        return;
                    }
                }

                if (auto notebook = makeNotebook_(fnxPath)) {
                    notebook->show();
                    notebook->beCute();
                }
            });
    }

    struct ParsedArgs_
    {
        Coco::PathList fnxFiles{};
        Coco::PathList regularFiles{};

        bool isEmpty() const noexcept
        {
            return fnxFiles.isEmpty() && regularFiles.isEmpty();
        }

        bool hasOnlyFnx() const noexcept
        {
            return !fnxFiles.isEmpty() && regularFiles.isEmpty();
        }

        bool hasOnlyRegular() const noexcept
        {
            return fnxFiles.isEmpty() && !regularFiles.isEmpty();
        }
    };

    ParsedArgs_ parseArgs_(const QStringList& args) const
    {
        ParsedArgs_ result{};

        // Skip Fernanda.exe
        for (auto i = 1; i < args.size(); ++i) {
            Coco::Path path(args.at(i));
            if (!path.exists() || path.isFolder()) continue;

            Fnx::Io::isFnxFile(path) ? result.fnxFiles << path
                                     : result.regularFiles << path;
        }

        return result;
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
