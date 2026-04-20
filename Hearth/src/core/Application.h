/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QApplication>
#include <QFontDatabase>
#include <QList>
#include <QSessionManager>
#include <QStringList>
#include <QTranslator>

#include <Coco/Path.h>

#include "core/AppDirs.h"
#include "core/BundledFonts.h"
#include "core/Debug.h"
#include "core/Files.h"
#include "core/LogViewer.h"
#include "core/Version.h"
#include "dialogs/BetaAlert.h"
#include "workspaces/Notebook.h"
#include "workspaces/Notepad.h"

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

    virtual ~Application() override
    {
        TRACER;
        AppDirs::cleanup();
    }

    void initialize()
    {
        if (initialized_) return;

        auto args = arguments();

        Debug::initialize(
            VERSION_IS_DEBUG || args.contains("--verbose"),
            AppDirs::logs(),
            VERSION_APP_NAME_STRING);

        if (args.contains("--log-viewer")) {
            new LogViewer; // Has delete on close attribute
        }

        initializeTranslator_();
        loadBundledFonts_();
        initializeNotepad_();

        // Let this block, so we don't interfere with any recovery prompt
        if (VERSION_IS_PRERELEASE) BetaAlert::exec();

        // Handle before args, in case an arg needs recovered instead
        maybeRecover_(); /// TODO BA
        handleFileArgs_(args);

        initialized_ = true;
        INFO(this, "Initialized");
        INFO("Version: {}", applicationVersion());
    }

public slots:
    void onStartCopAppRelaunched(const QStringList& args)
    {
        // These args are received via trying to reopen Fernanda (via, while
        // running, clicking the application, clicking associated file types, or
        // dragging a file onto the application icon)
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

    // TODO: Temp! Only handling EN for now
    void initializeTranslator_()
    {
        translator_ = new QTranslator(this);

        // Try beside the binary first (for Windows and macOS, plus Linux dev
        // build), then fallback to the FHS path on Linux only

        auto loaded =
            translator_->load("Translation_en.qm", applicationDirPath());

#ifdef Q_OS_LINUX

        if (!loaded) {
            loaded = translator_->load(
                "Translation_en.qm",
                applicationDirPath() + u"/../share/Fernanda/translations");
        }

#endif

        if (loaded) {
            INFO("Translation loaded!");
            if (installTranslator(translator_)) INFO("Translator installed!");
        } else {
            WARN("Failed to load translation: {}", "en");
            delete translator_;
            translator_ = nullptr;
        }
    }

    void loadBundledFonts_()
    {
        for (auto& path : BundledFonts::qrcPaths()) {
            if (QFontDatabase::addApplicationFont(path.toQString()) < 0) {
                WARN("Failed to load font: {}", path);
            }
        }
    }

    void initializeNotepad_()
    {
        notepad_ = new Notepad(this);
        connectWorkspace_(notepad_);

        connect(notepad_, &Notepad::lastWindowClosed, this, [this] {
            if (notebooks_.isEmpty()) quit();
        });
    }

    void handleFileArgs_(const QStringList& args)
    {
        auto parsed = parseArgs_(args);

        // Show notepad if we have regular files or nothing at all
        if (!parsed.regularFiles.isEmpty() || parsed.fnxFiles.isEmpty()) {
            auto was_open = notepad_->hasWindows();
            notepad_->show();
            notepad_->openFiles(parsed.regularFiles); // No-op if empty
            if (!was_open) notepad_->beCute();
        }

        for (auto& path : parsed.fnxFiles)
            openOrActivateNotebook_(path);
    }

    /// TODO BA
    void maybeRecover_()
    {
        // Notebooks: scan for orphaned lockfiles
        for (auto& lockfile : Coco::filePaths(
                 { AppDirs::tempNotebookRecovery() },
                 { "*" + NotebookLockfile::EXT })) {
            if (auto notebook = Notebook::recover(lockfile, this)) {
                registerNotebook_(notebook);
                notebook->show();
                notebook->beCute();
            } else {
                // Stale lockfile (working dir missing)
                Coco::remove(lockfile);
            }
        }

        // Notepad: check for recovery entries
        if (!Coco::paths(AppDirs::tempNotepadRecovery()).isEmpty()) {
            bool was_open = notepad_->hasWindows();
            notepad_->show();
            notepad_->recover();
            if (!was_open) notepad_->beCute();
        }
    }

    Notebook* makeNotebook_(const Coco::Path& fnxPath)
    {
        return registerNotebook_(new Notebook(fnxPath, this));
    }

    Notebook* registerNotebook_(Notebook* notebook)
    {
        if (!notebook) return nullptr;

        notebooks_ << notebook;
        connectWorkspace_(notebook);

        connect(notebook, &Notebook::lastWindowClosed, this, [this, notebook] {
            notebooks_.removeAll(notebook);
            delete notebook;
            if (notebooks_.isEmpty() && !notepad_->hasWindows()) quit();
        });

        connect(notebook, &Notebook::openNotepadRequested, this, [this] {
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
            [this](const Coco::Path& fnxPath) { openNotebook_(fnxPath); });

        connect(
            workspace,
            &Workspace::openNotebookRequested,
            this,
            [this](const Coco::Path& fnxPath) {
                /// TODO FT: This note is now maybe inconsistent with design!
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

        // Skip application binary
        for (auto i = 1; i < args.size(); ++i) {
            Coco::Path path(args.at(i));
            if (!path.exists() || path.isDir()) continue;

            Files::isFnxFile(path) ? result.fnxFiles << path
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
