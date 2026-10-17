/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QAction>
#include <QApplication>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QMessageBox>
#include <QSessionManager>
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "LogView.h"
#include "core/ActionIds.h"
#include "core/AppActions.h"
#include "core/AppConfig.h"
#include "core/AppDirs.h"
#include "core/BundledFonts.h"
#include "core/CMakeMacroGuard.h"
#include "core/Clargs.h"
#include "core/Publication.h"
#include "core/Translations.h"
#include "core/Updater.h"
#include "core/Vault.h"
#include "core/VaultEntry.h"
#include "core/Version.h"
#include "core/spell/SpellCheckers.h"
#include "ui/ManageVaults.h"
#include "ui/Style.h"
#include "ui/VaultWindow.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

class App : public QApplication
{
    Q_OBJECT

public:
    App(int& argc, char** argv)
        : QApplication(argc, argv)
    {
        setup_();
    }

    ~App() override { TRACER; }

    void init()
    {
        if (initialized_) {
            return;
        }

        // TODO: Open vaults and files passed as arguments
        auto args = arguments();

        initDebug_(args);

        // Before any window: a view built during session restore asks for a
        // bundled family (the default text font, Literata) as it's made, and
        // after logging so a font that fails to load is reported
        BundledFonts::registerFonts();

        // Before any window too: a text view asks for its vault's dictionary
        // as it's made
        SpellCheckers::installBundled(AppDirs::dictionaries());

        // suzuri.json is machine-local; a missing file (first run) leaves every
        // value at its AppDirs default. Load before the common vault so its
        // relocatable path is known when we construct it
        appConfig_.load();

        // Before any text is made, App's actions included, since text is
        // translated as it's made. The language comes from suzuri.json
        translations_.install(appConfig_.language());
        buildActions_();

        initCommonVault_();
        restoreSessionOrShowManageVaults_();

        updater_.start(
            appConfig_.checksForUpdates(),
            appConfig_.dismissedUpdate());

        initialized_ = true;
        DEBUG(this, "Initialized");
        DEBUG("Version: {}", applicationVersion());
    }

    // Build the vaults list on demand from the persisted recents: the single
    // source both surfaces read — ManageVaults' recents column directly, each
    // VaultSwitcher through a provider App threads down. name, missing, and
    // isCommon are derived here, once, rather than in each surface: name is the
    // folder's own name, missing is a stat (a recents path can point at a
    // folder since moved or deleted). Fresh each call — the list is short and
    // the surfaces rebuild on vaultsChanged / menu aboutToShow, so nothing to
    // cache
    [[nodiscard]] QList<VaultEntry> vaultEntries() const
    {
        const auto recents = appConfig_.recentVaults();

        // Asked of the live Vault, not of AppConfig: commonVault_ is the object
        // a rename would strand, so its own root is the honest comparand. A
        // null commonVault_ yields an empty path, which no recents path matches
        // — readVaults_ skips blank-path records
        const auto common_root =
            commonVault_ ? commonVault_->root() : Coco::Path{};

        QList<VaultEntry> entries{};
        entries.reserve(recents.size());

        // open is derived here alongside missing, but from the in-memory
        // openVaults_ registry rather than the disk: a recents path is "open"
        // when a window currently holds it. ManageVaults gates on it — a row's
        // rename and remove-from-list are disabled while that vault is open —
        // so the open-vault knowledge lives in one place. Fresh each call, so
        // it reflects opens AND closes; the surfaces rebuild on vaultsChanged,
        // which fires on both.
        //
        // NB: open is NOT a proxy for "a live Vault exists on this root". For a
        // project vault the two coincide — its Vault is constructed and
        // disposed with its window — but the Common Vault is App-owned and
        // outlives every window onto it, so a closed Common Vault row still has
        // a live Vault behind it. That is exactly why isCommon is a separate
        // field and not something a surface infers from open
        for (const auto& root : recents) {
            entries << VaultEntry{ root,
                                   root.nameQString(),
                                   !root.exists(),
                                   openVaults_.contains(root),
                                   root == common_root };
        }

        return entries;
    }

    // Whether the app quit to start again (the General page's Relaunch). Read
    // by main once the event loop has returned, after every window has closed
    // and saved
    [[nodiscard]] bool restartRequested() const noexcept
    {
        return restartRequested_;
    }

signals:
    // The vaults list OR its derived open-state changed — a vault opened,
    // closed, removed, or renamed. A live ManageVaults rebuilds its recents
    // column and re-evaluates its per-row gates (rename / remove-from-list);
    // the VaultSwitcher menus rebuild on aboutToShow instead, so they don't
    // need this. Fired from onOpenOrMakeVaultRequested_ (after save) on open,
    // from onVaultWindowDestroyed_ on close, and from the registry-edit slots
    // on remove / rename
    void vaultsChanged();

public slots:
    void onRelaunch(const QStringList& args)
    {
        // TODO: Hand a relaunch's arguments to the running instance
        (void)args;
    }

private:
    bool initialized_ = false;

    // Set by the graceful-exit handlers before they save the open set, and read
    // by onVaultWindowDestroyed_ so the teardown that follows doesn't clear the
    // very flags we just persisted
    bool quitting_ = false;

    // Set by onRestartRequested_ for main to read. Not to be confused with
    // onRelaunch, which is a second launch handing its arguments to this one
    bool restartRequested_ = false;

    // Set while checkVaultFolders_ has its message up, so switching away and
    // back meanwhile doesn't run the check again and stack a second box
    bool checkingVaultFolders_ = false;

    struct OpenVault_
    {
        Vault* vault = nullptr;
        Ui::VaultWindow* vaultWindow = nullptr;
    };

    AppConfig appConfig_{};
    Ui::ManageVaults* manageVaults_ = nullptr;

    Vault* commonVault_ = nullptr;
    QHash<Coco::Path, OpenVault_> openVaults_{};

    // The actions App owns and every window merely displays. Minted and
    // connected in buildActions_, filed under the ids the windows adopt them
    // by, and handed to each window at construction. Parented to App, so they
    // outlive every window
    AppActions appActions_{};

    // Every spelling dictionary in use, loaded once and shared by every
    // window. Windows borrow it; App outlives them all
    SpellCheckers spellCheckers_{ AppDirs::dictionaries() };

    // Whether a newer Suzuri has been released. Windows show what it found;
    // App saves its settings
    Updater updater_{};

    // The interface language. Installed in init; the General page changes the
    // setting, which App saves
    Translations translations_{};

    static void raiseWindow_(QWidget* window)
    {
        window->isMinimized() ? window->showNormal() : window->show();
        window->raise();
        window->activateWindow();
    }

    void setup_()
    {
        // NB: Logging will not work in this function! (It's initialized in
        // App::init, called after construction)

        setOrganizationName(PUB_AUTHOR_QSTRING);
        setOrganizationDomain(PUB_DOMAIN_QSTRING);
        setApplicationName(PUB_APP_NAME_QSTRING);
        setApplicationVersion(VERSION_FULL_QSTRING);
        setDesktopFileName(PUB_APP_ID_QSTRING); // Wayland app_id → .desktop

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)

        // NB: Windows takes its window icon from IDI_ICON1 (Suzuri.rc) and
        // macOS from the bundle's icns; X11 has neither
        QIcon icon{};

        for (auto size : { 16, 22, 24, 32, 48, 64, 128, 256, 512 }) {
            icon.addFile(u":/icons/Suzuri-%1.png"_s.arg(size));
        }

        setWindowIcon(icon);

#endif

        // Before any window, so every widget is made in Suzuri's style. It
        // replaces whatever style a -style argument asked for
        setStyle(new Ui::Style);

        setQuitOnLastWindowClosed(false);

        connect(this, &App::aboutToQuit, this, &App::onAboutToQuit_);

        connect(
            this,
            &App::commitDataRequest,
            this,
            &App::onCommitDataRequest_);

        connect(
            this,
            &App::applicationStateChanged,
            this,
            &App::onApplicationStateChanged_);

        connect(
            &updater_,
            &Updater::settingsChanged,
            this,
            &App::onUpdaterSettingsChanged_);

        connect(
            &translations_,
            &Translations::settingChanged,
            this,
            &App::onLanguageChanged_);
    }

    // Wire the App-owned actions, from init, once the translators are in. One
    // connect each, for the whole program: a window adopts these pointers to
    // display them and to bind their shortcuts, and never relays a triggered
    // signal back up. Adding one here is all it takes — every window adopts the
    // set wholesale.
    //
    // Quit must route through the graceful close path, which can still REFUSE
    // on a failed write — onQuitRequested_ does that; qApp->quit() would skip
    // it. Ctrl+Q explicitly rather than QKeySequence::Quit, which is empty on
    // Windows. A shortcut only fires once its action is on a widget, which
    // adoptAction does, so these are inert until a window exists — which is
    // correct
    void buildActions_()
    {
        auto* quit = new QAction(tr("Quit"), this);
        quit->setShortcut(QKeySequence(u"Ctrl+Q"_s));
        connect(quit, &QAction::triggered, this, &App::onQuitRequested_);
        appActions_.insert(ActionIds::APP_QUIT, quit);

        auto* manage_vaults = new QAction(tr("Manage Vaults"), this);
        connect(
            manage_vaults,
            &QAction::triggered,
            this,
            &App::openManageVaults_);
        appActions_.insert(ActionIds::APP_MANAGE_VAULTS, manage_vaults);

        auto* restart = new QAction(tr("Relaunch"), this);
        connect(restart, &QAction::triggered, this, &App::onRestartRequested_);
        appActions_.insert(ActionIds::APP_RESTART, restart);
    }

    void initDebug_(const QStringList& args)
    {
        auto verbose = APP_DEBUG || args.contains(Clargs::VERBOSE);
        Coco::Debug::init(verbose, AppDirs::logs(), PUB_APP_NAME_STRING);

        if (APP_DEBUG || args.contains(Clargs::LOG_VIEW)) {
            auto log_view = new LogView;
            log_view->show();
        }
    }

    void initCommonVault_()
    {
        // commonVaultPath() falls back to AppDirs::defaultCommonVault()
        // when unset
        commonVault_ = new Vault(appConfig_.commonVaultPath(), this);
    }

    // Restore last session's vaults: reopen every vault marked open whose
    // folder still exists, oldest-used first so the most-recent one activates
    // last and lands frontmost. open is the whole signal — a vault is marked
    // when opened and unmarked when closed (unless it's the last, or a global
    // quit kept it), so this reopens exactly what was up at the last exit. When
    // nothing qualifies — a fresh install, or a marked set whose folders all
    // vanished — show the picker rather than guessing a vault to open. exists()
    // is the per-candidate guard: any present folder is a valid vault, so only
    // a moved/deleted one is skipped. Each reopen reuses the one open/make
    // convergence point; the registry is path-keyed, so none early-returns as
    // already-open
    void restoreSessionOrShowManageVaults_()
    {
        auto restored = false;

        for (const auto& root : appConfig_.openVaultPaths()) {
            if (!root.exists()) {
                continue;
            }

            onOpenOrMakeVaultRequested_(root);
            restored = true;
        }

        if (!restored) {
            openManageVaults_();
        }
    }

    void openManageVaults_()
    {
        if (manageVaults_) {
            raiseWindow_(manageVaults_);
            return;
        }

        // Two live providers, read at click/refresh time rather than
        // snapshotted: the create/open MRU dir, and the MRO vaults list.
        // ManageVaults can outlive a vault open, so both must reflect App's
        // current state
        manageVaults_ = new Ui::ManageVaults(
            [this] { return appConfig_.newVaultMruDir(); },
            [this] { return vaultEntries(); },
            appActions_.value(ActionIds::APP_QUIT));

        connect(
            manageVaults_,
            &Ui::ManageVaults::openOrMakeVaultRequested,
            this,
            &App::onOpenOrMakeVaultRequested_);

        // Per-row actions on the recents column route back here:
        // remove-from-list and rename are registry edits App owns. Rename's
        // folder op already ran in ManageVaults; these slots only fix the
        // persisted state to match
        connect(
            manageVaults_,
            &Ui::ManageVaults::forgetVaultRequested,
            this,
            &App::onForgetVaultRequested_);

        connect(
            manageVaults_,
            &Ui::ManageVaults::vaultRenamed,
            this,
            &App::onVaultRenamed_);

        connect(
            manageVaults_,
            &QObject::destroyed,
            this,
            &App::onManageVaultsDestroyed_);

        // Keep a picker left open in sync when the list changes under it — a
        // vault opened from a CLI arg, or removed / renamed via a row action.
        // onOpenOrMakeVaultRequested_ emits vaultsChanged after each committed
        // open; manageVaults_ as the receiver auto-disconnects when it's
        // destroyed
        connect(
            this,
            &App::vaultsChanged,
            manageVaults_,
            &Ui::ManageVaults::refreshRecents);

        manageVaults_->show();
    }

    // The final-save sweep for every teardown that isn't a window close:
    // aboutToQuit and OS logout. flush() skips buffers already matching disk,
    // so calling this after the while-typing autosave already caught everything
    // is a cheap no-op. Failures can't be refused here (unlike a window close),
    // so the per-vault failure lists are discarded — writeModel_ already logged
    // them and left the buffers dirty
    void flushAllVaults_()
    {
        for (auto& entry : openVaults_) {
            if (entry.vault) {
                entry.vault->flush();
            }
        }

        if (commonVault_) {
            commonVault_->flush();
        }
    }

    // A vault's folder deleted or moved outside Suzuri. The user contract is
    // "don't do that", so nothing is rescued: an open project vault's window
    // closes — its buffers' files are gone, so its flush reports nothing, and
    // its workspace save is skipped (a missing root is never recreated) — and
    // the Common Vault's folder is made again, empty, since it's a standing
    // folder that startup would rebuild anyway. The Common Vault's own window,
    // if open, closes like any other: it's in openVaults_ under its root. One
    // message first, naming what happened — before anything closes, because
    // closing the last window starts the quit.
    //
    // Run when Suzuri becomes active again: deleting a vault's folder means
    // going to a file manager, and coming back is the moment to look. The
    // watchers can't be relied on here — on Windows Suzuri's own watches
    // often stop Explorer taking the folder at all, and where it does go,
    // whether the folder's own removal is reported varies by platform
    void checkVaultFolders_()
    {
        if (checkingVaultFolders_) {
            return;
        }

        // A window already closed by an earlier pass is hidden but stays in
        // openVaults_ until its deferred delete runs, and an activation can
        // land in between — skip it, or it'd be announced twice
        Coco::PathList missing_roots{};
        for (auto it = openVaults_.constBegin(); it != openVaults_.constEnd();
             ++it) {
            auto* window = it.value().vaultWindow;
            if (!it.key().isDir() && window && window->isVisible()) {
                missing_roots << it.key();
            }
        }

        const auto common_root =
            commonVault_ ? commonVault_->root() : Coco::Path{};
        auto common_missing = commonVault_ && !common_root.isDir();

        if (missing_roots.isEmpty() && !common_missing) {
            return;
        }

        checkingVaultFolders_ = true;

        QStringList lines{};
        for (const auto& root : missing_roots) {
            if (root == common_root) {
                continue; // named below, with what happens next
            }

            lines << tr("The folder for vault \"%1\" was moved or deleted "
                        "outside %2, so its window will close.")
                         .arg(root.nameQString(), PUB_APP_NAME_QSTRING);
        }

        if (common_missing) {
            lines << tr("The Common Vault folder was moved or deleted outside "
                        "%1. A new, empty one will be created at %2.")
                         .arg(
                             PUB_APP_NAME_QSTRING,
                             common_root.prettyQString());
        }

        QMessageBox::warning(
            nullptr,
            tr("Vault Folder Missing"),
            lines.join(u"\n\n"_s));

        // Looked up again after the message: the box ran an event loop. Each
        // close runs the window's ordinary closeEvent, then WA_DeleteOnClose
        // and onVaultWindowDestroyed_ — which quits if it was the last
        for (const auto& root : missing_roots) {
            if (auto it = openVaults_.constFind(root);
                it != openVaults_.constEnd() && it.value().vaultWindow) {
                it.value().vaultWindow->close();
            }
        }

        // After the Common Vault's own window has closed, so its workspace
        // save finds no folder and writes nothing into the new one
        if (common_missing) {
            commonVault_->recreateRoot();
        }

        checkingVaultFolders_ = false;
    }

    // The workspace twin of flushAllVaults_: the exit paths that DON'T run each
    // window's closeEvent — a multi-window quit, an OS logout — still need
    // every open window's workspace.json written. saveWorkspaceNow flushes each
    // window's pending debounce synchronously. In the ordinary single-window
    // path this iterates nothing (the window already saved and left openVaults_
    // via its closeEvent), exactly like flushAllVaults_
    void saveAllWorkspaces_()
    {
        for (auto& entry : openVaults_) {
            if (entry.vaultWindow) {
                entry.vaultWindow->saveWorkspaceNow();
            }
        }
    }

    void onOpenOrMakeVaultRequested_(const Coco::Path& vaultRoot)
    {
        if (auto it = openVaults_.constFind(vaultRoot);
            it != openVaults_.constEnd()) {
            raiseWindow_(it.value().vaultWindow);
            return;
        }

        // One Vault per root. The common root opened as its own window reuses
        // App's common Vault rather than building a second over the same
        // folder: two Vaults on one root means two models per file, so a common
        // file open in a project window (through commonVault_) and in this
        // window would be two buffers, each autosaving over the other. With one
        // Vault this window's vault_ IS its commonVault_, and every "is this
        // the Common Vault?" question downstream — the Sidebar's drawer, the
        // FileSwitcher's second listing, the tab indicator — answers itself
        auto* vault = (vaultRoot == commonVault_->root())
                          ? commonVault_
                          : new Vault(vaultRoot, this);

        auto window = new Ui::VaultWindow(
            vault,
            commonVault_,
            [this] { return vaultEntries(); },
            appActions_,
            &spellCheckers_,
            &updater_,
            &translations_);

        openVaults_.insert(vaultRoot, { vault, window });

        // Every committed create/open converges here, so this is the one
        // authoritative site for the persisted app state that tracks vault use.
        // One save covers three records: the new-vault MRU (the vault's PARENT
        // — the dir to start the next create/open in); this vault's last-used
        // timestamp, which touchVault bumps (upserting a first-time vault) to
        // order the recents list; and its open flag, set here and cleared on
        // close, so next launch restores the session. Only the construct branch
        // reaches here, so re-raising an already-open vault (the early return
        // above) doesn't run this — though its activation still bumps the
        // timestamp (onVaultWindowActivated_). vaultsChanged then refreshes any
        // surface showing the list
        appConfig_.setNewVaultMruDir(vaultRoot.parent());
        appConfig_.touchVault(vaultRoot);
        appConfig_.setVaultOpen(vaultRoot, true);
        appConfig_.save();

        emit vaultsChanged();

        connect(window, &QObject::destroyed, this, [this, vaultRoot] {
            onVaultWindowDestroyed_(vaultRoot);
        });

        // Follow focus so recency tracks the vault the user is actually in, not
        // just the last opened. Memory-only per activation; persisted at exit
        connect(
            window,
            &Ui::VaultWindow::windowActivated,
            this,
            [this, vaultRoot] { onVaultWindowActivated_(vaultRoot); });

        // A vault chosen from this window's switcher. Straight into the one
        // convergence point — switching to an open vault raises it, to a closed
        // one opens it, same as the picker
        connect(
            window,
            &Ui::VaultWindow::vaultSwitchRequested,
            this,
            &App::onOpenOrMakeVaultRequested_);

        window->show();

        if (manageVaults_) {
            manageVaults_->close();
        }
    }

    void onVaultWindowDestroyed_(const Coco::Path& vaultRoot)
    {
        auto entry = openVaults_.take(vaultRoot);

        // ~QObject emits destroyed BEFORE deleting its children, so the
        // window's views may still be alive and holding models this Vault
        // owns. The common Vault is App's, not the window's: project windows
        // still hold its models, and it outlives every window onto it, so
        // closing its own window never disposes it
        if (entry.vault && entry.vault != commonVault_) {
            entry.vault->deleteLater();
        }

        // Maintain the persisted open set on ordinary closes only. Clear this
        // vault's flag when another vault is still open; leave it set when it's
        // the last one closing, so it stays the reopen anchor (Obsidian marks
        // the last-used vault and never clears it on close). On a global quit
        // (quitting_) the exit handler already saved every open vault as open —
        // clearing here as the windows tear down would erase exactly the set
        // restore needs, so skip it
        if (!quitting_ && !openVaults_.isEmpty()) {
            appConfig_.setVaultOpen(vaultRoot, false);
            appConfig_.save();
        }

        // A close changes derived open-state, so a picker left up re-enables
        // this row's rename / remove-from-list. take() ran first, so
        // vaultEntries() already sees this vault as closed
        emit vaultsChanged();

        // Last vault gone. Quit unless the picker is up for the user to reopen
        if (openVaults_.isEmpty() && !manageVaults_) {
            quit();
        }
    }

    // A VaultWindow came to the front (initial show, alt-tab, click). Bump its
    // last-used timestamp so it sorts to the front of the recents list and, on
    // restore, stacks frontmost — the deliberate "last-used" (not merely
    // last-opened) choice, matching Obsidian's
    // switch-to-a-vault-makes-it-recent. This names no reopen target — the open
    // flag does that — it's purely recency for ordering. Memory only:
    // activation fires on every focus change and the timestamp is read only at
    // launch, so persisting each would churn suzuri.json for nothing; it
    // reaches disk at exit (onAboutToQuit_/ onCommitDataRequest_). Pop-out
    // activation isn't tracked; recency is vault-granular and the main window
    // covers the common case
    void onVaultWindowActivated_(const Coco::Path& vaultRoot)
    {
        appConfig_.touchVault(vaultRoot);
    }

    void onForgetVaultRequested_(const Coco::Path& vaultRoot)
    {
        // The picker gates this to closed vaults, but guard anyway — forgetting
        // an open vault would desync the list from openVaults_
        if (openVaults_.contains(vaultRoot)) {
            return;
        }

        // One erase, no list surgery. forgetVault reports whether anything was
        // removed, so an unknown path is a no-op rather than a spurious save +
        // signal
        if (!appConfig_.forgetVault(vaultRoot)) {
            return;
        }

        appConfig_.save();

        emit vaultsChanged();
    }

    // ManageVaults already renamed the FOLDER on disk; App fixes the registry
    // to match. One in-place path swap — lastUsedEpochMs and open ride along,
    // so the entry keeps its recents position (a rename isn't a use) and its
    // open/anchor role if it had one. Rename is gated to closed vaults, so no
    // live Vault or window is keyed on oldRoot to re-point. save +
    // vaultsChanged refreshes the list with the new name
    void onVaultRenamed_(const Coco::Path& oldRoot, const Coco::Path& newRoot)
    {
        appConfig_.renameVault(oldRoot, newRoot);
        appConfig_.save();

        emit vaultsChanged();
    }

    void onManageVaultsDestroyed_()
    {
        manageVaults_ = nullptr;

        if (openVaults_.isEmpty()) {
            quit();
        }
    }

    // A window asked to quit the whole app — Ctrl+Q or File → Quit, from any
    // window. Route through each open window's closeEvent, the one path that
    // flushes, saves workspace, and can still REFUSE on a failed write; never
    // qApp->quit(), which skips all three. Set quitting_ first so the
    // per-window destroyed handlers preserve the open set for next-launch
    // restore, exactly as onAboutToQuit_ does; on a refusal we aren't quitting
    // after all, so clear it again. close() returns false when a window refuses
    // — stop there and leave the app running rather than quit past unsaved
    // work. The last window's destroyed fires quit() via
    // onVaultWindowDestroyed_ / onManageVaultsDestroyed_, so we don't call
    // quit() here; we only make sure a picker that's the sole remaining window
    // also closes
    void onQuitRequested_()
    {
        quitting_ = true;

        const auto entries = openVaults_.values();
        for (const auto& entry : entries) {
            if (entry.vaultWindow && !entry.vaultWindow->close()) {
                quitting_ = false;
                return;
            }
        }

        if (manageVaults_) {
            manageVaults_->close();
        }
    }

    // Quit by the usual path, then main starts Suzuri again once this copy is
    // gone. A quit refused over a failed write starts nothing
    void onRestartRequested_()
    {
        restartRequested_ = true;
        onQuitRequested_();

        if (!quitting_) {
            restartRequested_ = false;
        }
    }

    // Obsidian flushes on focus loss; mirror it. Mostly redundant with the
    // debounce (defocusing is itself a pause), but it closes the sliver where
    // you switch away within the debounce window, and it's a boundary a writer
    // hits constantly. flush() returns per-vault failure lists — discarded
    // here: writeModel_ already logged them, and an autosave-class trigger
    // doesn't prompt. The dirty buffers stay dirty and retry.
    //
    // Coming back, check every vault's folder is still there
    void onApplicationStateChanged_(Qt::ApplicationState state)
    {
        if (state == Qt::ApplicationActive) {
            checkVaultFolders_();
            return;
        }

        if (state != Qt::ApplicationInactive) {
            return;
        }

        flushAllVaults_();
    }

    // The updater's two settings live in suzuri.json, written when either
    // changes
    void onUpdaterSettingsChanged_()
    {
        appConfig_.setChecksForUpdates(updater_.checksAutomatically());
        appConfig_.setDismissedUpdate(updater_.dismissedVersion());
        appConfig_.save();
    }

    void onLanguageChanged_()
    {
        appConfig_.setLanguage(translations_.setting());
        appConfig_.save();
    }

    void onAboutToQuit_()
    {
        // Every clean exit funnels through quit(), so this fires on all of them
        // — the last-window path, the Quit action, Cmd-Q. Mark quitting_ FIRST:
        // it tells the window-destroyed handlers firing during teardown to
        // leave the open flags alone, so the open set we save just below
        // survives to next launch. Then flush everything — in the normal path
        // the project vaults were already flushed and removed by their
        // closeEvents, so this reduces to the windowless common vault; a
        // still-open window is caught too. The save persists the open set and
        // the last-used timestamps bumped in memory on activation since the
        // last open
        quitting_ = true;

        flushAllVaults_();
        saveAllWorkspaces_();
        appConfig_.save();
    }

    // TODO: Test this path (an OS logout or shutdown)
    void onCommitDataRequest_([[maybe_unused]] QSessionManager& manager)
    {
        // OS logout/shutdown terminates us without delivering a single
        // closeEvent, so this is where open vaults get their final write — and
        // the canonical multi-vault exit, since it pre-empts the
        // close-each-window path with several vaults still open. Best-effort:
        // we save rather than cancel the shutdown — a failed write is logged
        // and the dirty buffer dies with us, which no amount of blocking would
        // prevent. No quitting_ set here: any window teardown that could clear
        // the open flags runs during ~QApplication, after exec() returns, after
        // aboutToQuit — so onAboutToQuit_ has already set it. If the SM instead
        // hard-kills us right after this returns, nothing is destroyed and
        // there's nothing to guard
        flushAllVaults_();
        saveAllWorkspaces_();
        appConfig_.save();
    }
};

} // namespace Fernanda
