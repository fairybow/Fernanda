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
#include <QTranslator>

#include "Coco/Path.h"

#include "AppDirs.h"
#include "BetaAlert.h"
#include "Debug.h"
#include "FileTypes.h"
#include "Notebook.h"
#include "Notepad.h"
#include "Timers.h"
#include "Version.h"

namespace Fernanda {

// Top-level application coordinator and entry point that creates and manages
// Workspaces and handles application lifecycle
// TODO: Session handling
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

        Debug::initialize(Version::isDebug); // TODO: Log file path
        if (!AppDirs::initialize()) FATAL("App directory creation failed!");

        initializeTranslator_();
        loadBundledFonts_();
        initializeNotepad_();
        handleArgs_();

        initialized_ = true;

        if (Version::isPrerelease) {
            Timers::delay(500, this, [] { BetaAlert::exec(); });
        }
    }

public slots:
    void onStartCopAppRelaunched(const QStringList& args)
    {
        // These args are received via trying to reopen Fernanda (via, while
        // running, clicking the EXE, clicking associated file types, or
        // dragging a file onto the EXE)
        auto parsed = parseArgs_(args);

        if (parsed.isEmpty()) {
            notepad_->activate();
            for (auto& notebook : notebooks_)
                notebook->activate();
            return;
        }

        if (!parsed.regularFiles.isEmpty()) {
            notepad_->show();
            notepad_->openFiles(parsed.regularFiles);
        }

        for (auto& path : parsed.fnxFiles)
            openOrActivateNotebook_(path);
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
    struct ParsedArgs_
    {
        Coco::PathList fnxFiles{};
        Coco::PathList regularFiles{};

        bool isEmpty() const noexcept
        {
            return fnxFiles.isEmpty() && regularFiles.isEmpty();
        }
    };

    bool initialized_ = false;
    QTranslator* translator_ = nullptr;
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

    void initializeTranslator_()
    {
        translator_ = new QTranslator(this);

        if (translator_->load(":/qm/Translation_en.qm")) {
            INFO("Translation loaded!");
            if (installTranslator(translator_)) INFO("Translator installed!");
        } else {
            // TODO: Only handling EN for now
            WARN("Failed to load translation: {}", "en");
            delete translator_;
            translator_ = nullptr;
        }
    }

    void loadBundledFonts_()
    {
        for (auto& path : Coco::filePaths(
                 { ":/mononoki/", ":/opendyslexic/" },
                 { "*.otf" })) {
            if (QFontDatabase::addApplicationFont(path.toQString()) < 0)
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

    void handleArgs_()
    {
        auto parsed = parseArgs_(arguments());

        // Show notepad if we have regular files or nothing at all
        if (!parsed.regularFiles.isEmpty() || parsed.fnxFiles.isEmpty()) {
            notepad_->show();
            notepad_->openFiles(parsed.regularFiles); // No-op if empty
            notepad_->beCute();
        }

        // Make a Notebook for each FNX file (and open single window for each)
        for (auto& path : parsed.fnxFiles)
            openNotebook_(path);
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
            [&](const Coco::Path& fnxPath) { openNotebook_(fnxPath); });

        connect(
            workspace,
            &Workspace::openNotebookRequested,
            this,
            [&](const Coco::Path& fnxPath) {
                // Shouldn't need to check Fnx::isFnxFile. The promise of this
                // signal is "open Notebook" not "open maybe a Notebook"!
                // TODO: Although, we may need to do some redesign if we want to
                // prompt for files that are .fnx by extension only...
                openOrActivateNotebook_(fnxPath);
            });
    }

    ParsedArgs_ parseArgs_(const QStringList& args) const
    {
        ParsedArgs_ result{};

        // Skip Fernanda.exe
        for (auto i = 1; i < args.size(); ++i) {
            Coco::Path path(args.at(i));
            if (!path.exists() || path.isDir()) continue;

            Fnx::Io::isFnxFile(path) ? result.fnxFiles << path
                                     : result.regularFiles << path;
        }

        return result;
    }

    void openNotebook_(const Coco::Path& fnxPath)
    {
        if (auto notebook = makeNotebook_(fnxPath)) {
            notebook->show();
            notebook->beCute();
        }
    }

    void openOrActivateNotebook_(const Coco::Path& fnxPath)
    {
        for (auto& notebook : notebooks_) {
            if (notebook->fnxPath() == fnxPath) {
                notebook->activate();
                return;
            }
        }

        openNotebook_(fnxPath);
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
