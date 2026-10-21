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
#include <Coco/Time.h>

#include "LogView.h"
#include "core/ActionIds.h"
#include "core/AppActions.h"
#include "core/AppConfig.h"
#include "core/AppDirs.h"
#include "core/BufferStore.h"
#include "core/BundledFonts.h"
#include "core/CMakeMacroGuard.h"
#include "core/Clargs.h"
#include "core/PathKey.h"
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

        // fernanda.json is machine-local; a missing file (first run) leaves
        // every value at its AppDirs default. Loaded before the Common Vault,
        // which it names
        appConfig_.load();

        // Before any text is made, App's actions included, since text is
        // translated as it's made. The language comes from fernanda.json
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
        // null commonVault_ yields an empty key, which no recents path matches
        // — readVaults_ skips blank-path records, and an empty key is never
        // taken for a match below
        const auto common_key =
            commonVault_ ? commonVault_->rootKey() : PathKey{};

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
            auto key = PathKey::of(root);

            entries << VaultEntry{ root,
                                   root.nameQString(),
                                   !root.exists(),
                                   openVaults_.contains(key),
                                   !key.isEmpty() && key == common_key };
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
    // from onVaultWindowDestroyed_ on close, from the registry-edit slots on
    // remove / rename, and from moveVaultRecords_ when an in-app rename or move
    // takes known vaults' folders with it
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

    // Every open file's buffer, for every vault and window. Made before any
    // Vault, which is given it, and a child of App, so it outlives every
    // window and every view of its buffers
    BufferStore* bufferStore_ = new BufferStore(this);

    Vault* commonVault_ = nullptr;

    // Keyed by each vault's root as a PathKey, so every spelling of one folder
    // finds the one entry, letter case included where the platform ignores it
    QHash<PathKey, OpenVault_> openVaults_{};

    // The actions App owns and every window merely displays. Minted and
    // connected in buildActions_, filed under the ids the windows adopt them
    // by, and handed to each window at construction. Parented to App, so they
    // outlive every window
    AppActions appActions_{};

    // Every spelling dictionary in use, loaded once and shared by every
    // window. Windows borrow it; App outlives them all
    SpellCheckers spellCheckers_{ AppDirs::dictionaries() };

    // Whether a newer Fernanda has been released. Windows show what it found;
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

    // A vault folder as App keys it: resolved through symlinks, so one folder
    // reached two ways is one Vault, and every file key made under it starts
    // from the same spelling. A folder that isn't there yet can't be resolved,
    // and is only normalized. Either way it's spelled as PathKey spells paths
    [[nodiscard]] static Coco::Path resolvedRoot_(const Coco::Path& root)
    {
        auto resolved = root.canonical();
        return PathKey::of(resolved.isEmpty() ? root : resolved).path();
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

        // NB: Windows takes its window icon from IDI_ICON1 (Fernanda.rc) and
        // macOS from the bundle's icns; X11 has neither
        QIcon icon{};

        for (auto size : { 16, 22, 24, 32, 48, 64, 128, 256, 512 }) {
            icon.addFile(u":/icons/Fernanda-%1.png"_s.arg(size));
        }

        setWindowIcon(icon);

#endif

        // Before any window, so every widget is made in Fernanda's style. It
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

        // Handled a tick later (onVaultFoldersLeft_ says why)
        connect(
            bufferStore_,
            &BufferStore::entryRelocated,
            this,
            [this](const PathKey& oldEntry, const PathKey& newEntry) {
                Coco::Time::onNextTick(this, [this, oldEntry, newEntry] {
                    onEntryRelocated_(oldEntry, newEntry);
                });
            });

        connect(
            bufferStore_,
            &BufferStore::entryRemoved,
            this,
            [this](const PathKey& entry) {
                Coco::Time::onNextTick(this, [this, entry] {
                    onVaultFoldersLeft_(entry, PathKey{});
                });
            });

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

    // The Common Vault, if there is one. The first run sets one up, made at
    // AppDirs::defaultCommonVault; after that there is one only while the
    // user has one set. A set one whose folder is gone is unset, and stays
    // unset until the user sets another: the folder was deleted or moved
    // outside Fernanda, and where it went can't be known. Its record stays,
    // listed as missing
    void initCommonVault_()
    {
        if (!appConfig_.isCommonVaultSetUp()) {
            appConfig_.setCommonVaultPath(
                resolvedRoot_(AppDirs::defaultCommonVault()));
            appConfig_.save();
        }

        auto path = appConfig_.commonVaultPath();
        if (path.isEmpty()) {
            return;
        }

        auto root = resolvedRoot_(path);

        if (!root.isDir()) {
            INFO("Common Vault folder {} is gone; unsetting it", root);
            appConfig_.setCommonVaultPath({});
            appConfig_.save();
            return;
        }

        commonVault_ = new Vault(root, bufferStore_, this);
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

    // The final-save sweep for every teardown that isn't a window close —
    // aboutToQuit and OS logout — and for focus loss: every buffer, then every
    // vault's pending settings. The flush skips buffers already matching disk,
    // so calling this after the while-typing autosave already caught everything
    // is a cheap no-op. Failures can't be refused here (unlike a window close),
    // so the failure list is discarded — the store already logged them and
    // left the buffers dirty
    void flushAll_()
    {
        bufferStore_->flush();

        for (auto& entry : openVaults_) {
            if (entry.vault) {
                entry.vault->saveSettingsNow();
            }
        }

        if (commonVault_) {
            commonVault_->saveSettingsNow();
        }
    }

    // A vault's folder deleted or moved outside Fernanda. The user contract is
    // "don't do that", so nothing is rescued: an open project vault's window
    // closes — its buffers' files are gone, so its flush reports nothing, and
    // its workspace save is skipped (a missing root is never recreated) — and
    // the Common Vault's folder is made again, empty, since it's a standing
    // folder that startup would rebuild anyway. The Common Vault's own window,
    // if open, closes like any other: it's in openVaults_ under its root. One
    // message first, naming what happened — before anything closes, because
    // closing the last window starts the quit.
    //
    // Run when Fernanda becomes active again: deleting a vault's folder means
    // going to a file manager, and coming back is the moment to look. The
    // watchers can't be relied on here — on Windows Fernanda's own watches
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
        QList<PathKey> missing_roots{};
        for (auto it = openVaults_.constBegin(); it != openVaults_.constEnd();
             ++it) {
            auto* window = it.value().vaultWindow;
            if (!it.key().path().isDir() && window && window->isVisible()) {
                missing_roots << it.key();
            }
        }

        const auto common_key =
            commonVault_ ? commonVault_->rootKey() : PathKey{};
        const auto common_root = common_key.path();
        auto common_missing = commonVault_ && !common_root.isDir();

        if (missing_roots.isEmpty() && !common_missing) {
            return;
        }

        QStringList lines{};
        for (const auto& root : missing_roots) {
            if (root == common_key) {
                continue; // named below, with what happens next
            }

            lines << tr("The folder for vault \"%1\" was moved or deleted "
                        "outside %2, so its window will close.")
                         .arg(root.path().nameQString(), PUB_APP_NAME_QSTRING);
        }

        if (common_missing) {
            lines << tr("The Common Vault folder was moved or deleted outside "
                        "%1. A new, empty one will be created at %2.")
                         .arg(
                             PUB_APP_NAME_QSTRING,
                             common_root.prettyQString());
        }

        closeVaultsWithoutFolders_(
            tr("Vault Folder Missing"),
            lines,
            missing_roots,
            common_missing);
    }

    // A Vault's rename or move took entry to newEntry. Vaults nest, so entry
    // may have held other vaults' folders. Vaults whose folders are still in
    // place take the new spelling, the windows of those whose folders left
    // close, and the known-vaults records of those follow their folders.
    //
    // In that order because the message before the windows close runs an
    // event loop, in which any vault window can be activated, and activation
    // records a vault's use by its root's path (touchVault, which adds a
    // record it doesn't find). Until a window closes, its vault's record must
    // stay where the vault's root is
    void onEntryRelocated_(const PathKey& entry, const PathKey& newEntry)
    {
        respellVaults_(entry, newEntry);
        onVaultFoldersLeft_(entry, newEntry);
        moveVaultRecords_(entry, newEntry);
    }

    // Where letter case is ignored, renaming a folder by case alone leaves
    // every vault inside it in place, spelled the old way. Each such vault
    // takes the new spelling: its known-vaults record, open or not, and every
    // live Vault (Vault::respellRoot), the Common Vault and each vault with a
    // window. A Vault's tree and every window's copy of its root follow
    // through rootRespelled. Then openVaults_ is keyed again: a QHash keeps the
    // key an entry was inserted with, and onVaultWindowDestroyed_ reads the
    // record's path from it.
    //
    // The Common Vault's new spelling is also kept as its path in AppConfig.
    // Its root is built from that path at each launch, so the next launch
    // spells it as this one now does, and as its record does
    void respellVaults_(const PathKey& entry, const PathKey& newEntry)
    {
        auto config_changed = false;

        for (const auto& root : appConfig_.recentVaults()) {
            auto key = PathKey::of(root);

            if (key.isAtOrUnder(entry) && root.isDir()) {
                appConfig_.renameVault(
                    root,
                    key.rebased(entry, newEntry).path());
                config_changed = true;
            }
        }

        auto respell = [&entry, &newEntry](Vault* vault) {
            auto key = vault->rootKey();

            if (key.isAtOrUnder(entry) && vault->root().isDir()) {
                vault->respellRoot(key.rebased(entry, newEntry));
            }
        };

        if (commonVault_) {
            auto old_root = commonVault_->root();
            respell(commonVault_);

            if (commonVault_->root() != old_root) {
                appConfig_.setCommonVaultPath(commonVault_->root());
                config_changed = true;
            }
        }

        QList<PathKey> respelled{};

        for (auto it = openVaults_.constBegin(); it != openVaults_.constEnd();
             ++it) {
            auto* vault = it.value().vault;

            if (!vault) {
                continue;
            }

            respell(vault);

            if (it.key().path() != vault->root()) {
                respelled << it.key();
            }
        }

        for (const auto& key : respelled) {
            auto open_vault = openVaults_.take(key);
            openVaults_.insert(open_vault.vault->rootKey(), open_vault);
        }

        if (config_changed) {
            appConfig_.save();
            emit vaultsChanged();
        }
    }

    // The known-vaults record of each vault whose folder left with entry, open
    // or not, takes its folder's new path and keeps its place in the recents.
    // So the vault switchers and Manage Vaults list a moved vault at its new
    // place, not as missing.
    //
    // A moved vault isn't followed: its window has closed
    // (onVaultFoldersLeft_), and it's opened again from its new place like
    // any other. Its open flag is cleared here, since that window's own close
    // looks for the record under the old path and finds nothing. The Common
    // Vault's record stays, since its place is fixed and a new Common Vault is
    // made there; the folder that left is an ordinary folder now
    void moveVaultRecords_(const PathKey& entry, const PathKey& newEntry)
    {
        const auto common_key =
            commonVault_ ? commonVault_->rootKey() : PathKey{};

        auto changed = false;

        for (const auto& root : appConfig_.recentVaults()) {
            auto key = PathKey::of(root);

            if (!key.isAtOrUnder(entry) || root.isDir() || key == common_key) {
                continue;
            }

            auto new_root = key.rebased(entry, newEntry).path();
            appConfig_.renameVault(root, new_root);
            appConfig_.setVaultOpen(new_root, false);
            changed = true;
        }

        if (changed) {
            appConfig_.save();
            emit vaultsChanged();
        }
    }

    // The in-app twin of checkVaultFolders_: a Vault's rename, move, or trash
    // of entry took other vaults' folders with it. Vaults nest, so a folder
    // inside one vault may hold another vault's folder, or the Common Vault's.
    // Each is handled as a folder moved or deleted outside Fernanda: a project
    // vault's window closes, and the Common Vault's folder is made again,
    // empty, in its place, while the files that were in it stay where they
    // went. Only the message differs, saying where each folder went. newEntry
    // is where entry went, or empty for a trash.
    //
    // A vault folder still there afterward stays open: where letter case is
    // ignored, a change of case alone leaves every vault inside it in place,
    // and respellVaults_ has already given it the new spelling. Called a tick
    // after BufferStore announces the operation, so the Vault and the window
    // that acted have finished before any window closes
    void onVaultFoldersLeft_(const PathKey& entry, const PathKey& newEntry)
    {
        const auto common_key =
            commonVault_ ? commonVault_->rootKey() : PathKey{};

        QList<PathKey> roots{};
        QStringList lines{};

        for (auto it = openVaults_.constBegin(); it != openVaults_.constEnd();
             ++it) {
            const auto& root = it.key();
            auto* window = it.value().vaultWindow;

            if (!window || !window->isVisible() || !root.isAtOrUnder(entry) ||
                root.path().isDir()) {
                continue;
            }

            roots << root;

            if (root == common_key) {
                continue; // named below, with what happens next
            }

            auto name = root.path().nameQString();

            if (newEntry.isEmpty()) {
                lines << tr("The folder for vault \"%1\" was moved to the "
                            "trash, so its window will close.")
                             .arg(name);
            } else {
                lines << tr("The folder for vault \"%1\" was moved to %2, so "
                            "its window will close.")
                             .arg(
                                 name,
                                 root.rebased(entry, newEntry)
                                     .path()
                                     .prettyQString());
            }
        }

        auto common_gone = commonVault_ && common_key.isAtOrUnder(entry) &&
                           !common_key.path().isDir();

        if (common_gone) {
            auto common_root = common_key.path().prettyQString();

            if (newEntry.isEmpty()) {
                lines << tr("The Common Vault folder was moved to the trash. A "
                            "new, empty one will be created at %1.")
                             .arg(common_root);
            } else {
                lines << tr("The Common Vault folder was moved to %1, and its "
                            "files are there now. A new, empty Common Vault "
                            "will be created at %2.")
                             .arg(
                                 common_key.rebased(entry, newEntry)
                                     .path()
                                     .prettyQString(),
                                 common_root);
            }
        }

        if (lines.isEmpty()) {
            return;
        }

        closeVaultsWithoutFolders_(
            tr("Vault Folder Moved"),
            lines,
            roots,
            common_gone);
    }

    // Say in one message what happened, then close the window of each vault
    // in roots and, when recreateCommon, make the Common Vault's folder again.
    // The message comes before anything closes, because closing the last
    // window starts the quit. checkingVaultFolders_ is set while it's up, so
    // switching away and back meanwhile doesn't run checkVaultFolders_ and
    // stack a second one
    void closeVaultsWithoutFolders_(
        const QString& title,
        const QStringList& lines,
        const QList<PathKey>& roots,
        bool recreateCommon)
    {
        checkingVaultFolders_ = true;

        QMessageBox::warning(nullptr, title, lines.join(u"\n\n"_s));

        // Looked up again after the message: the box ran an event loop. Each
        // close runs the window's ordinary closeEvent, then WA_DeleteOnClose
        // and onVaultWindowDestroyed_ — which quits if it was the last
        for (const auto& root : roots) {
            if (auto it = openVaults_.constFind(root);
                it != openVaults_.constEnd() && it.value().vaultWindow) {
                it.value().vaultWindow->close();
            }
        }

        // After the Common Vault's own window has closed, so its workspace
        // save finds no folder and writes nothing into the new one
        if (recreateCommon) {
            recreateCommonVault_();
        }

        checkingVaultFolders_ = false;
    }

    // Make the Common Vault's folder again, at its usual place and in its usual
    // spelling, which replaces any spelling App kept for the folder that's
    // gone (respellVaults_). Its record takes the same spelling; the record is
    // matched by key, so renameVault finds it however it's spelled
    void recreateCommonVault_()
    {
        auto root = resolvedRoot_(AppDirs::defaultCommonVault());
        appConfig_.setCommonVaultPath(root);

        commonVault_->recreateRoot(PathKey::of(root));

        appConfig_.renameVault(root, root);
        appConfig_.save();

        emit vaultsChanged();
    }

    // The workspace twin of flushAll_: the exit paths that DON'T run each
    // window's closeEvent — a multi-window quit, an OS logout — still need
    // every open window's workspace.json written. saveWorkspaceNow flushes each
    // window's pending debounce synchronously. In the ordinary single-window
    // path this iterates nothing (the window already saved and left openVaults_
    // via its closeEvent), as flushAll_ reduces to the buffers no window
    // flushed
    void saveAllWorkspaces_()
    {
        for (auto& entry : openVaults_) {
            if (entry.vaultWindow) {
                entry.vaultWindow->saveWorkspaceNow();
            }
        }
    }

    // requestedRoot is resolved first, so the known-vaults list records one
    // spelling per folder, and the open-vaults map and the common-root check
    // below compare keys, so any spelling of an open folder finds it
    void onOpenOrMakeVaultRequested_(const Coco::Path& requestedRoot)
    {
        auto vaultRoot = resolvedRoot_(requestedRoot);
        auto root_key = PathKey::of(vaultRoot);

        if (auto it = openVaults_.constFind(root_key);
            it != openVaults_.constEnd()) {
            raiseWindow_(it.value().vaultWindow);
            return;
        }

        // One Vault per root. The common root opened as its own window reuses
        // App's common Vault rather than building a second over the same
        // folder, which would split the folder's settings and dictionary
        // between two objects. With one Vault this window's vault_ IS its
        // commonVault_, and every "is this the Common Vault?" question
        // downstream — the Sidebar's drawer, the FileSwitcher's second listing,
        // the tab indicator — answers itself
        auto* vault = (commonVault_ && root_key == commonVault_->rootKey())
                          ? commonVault_
                          : new Vault(vaultRoot, bufferStore_, this);

        auto window = new Ui::VaultWindow(
            vault,
            commonVault_,
            bufferStore_,
            [this] { return vaultEntries(); },
            appActions_,
            &spellCheckers_,
            &updater_,
            &translations_);

        openVaults_.insert(root_key, { vault, window });

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

        connect(window, &QObject::destroyed, this, [this, root_key] {
            onVaultWindowDestroyed_(root_key);
        });

        // Follow focus so recency tracks the vault the user is actually in, not
        // just the last opened. Memory-only per activation; persisted at exit
        // The vault's root is asked for each time, since respellVaults_ can
        // change its spelling, and the record with it
        connect(window, &Ui::VaultWindow::windowActivated, this, [this, vault] {
            onVaultWindowActivated_(vault->root());
        });

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

    // The record's path is the open-vaults map's key, not rootKey: both are
    // spelled as onOpenOrMakeVaultRequested_ recorded the vault in AppConfig,
    // but a vault respelled while open (respellVaults_) has its record and its
    // map key respelled with it, while rootKey keeps the old spelling
    void onVaultWindowDestroyed_(const PathKey& rootKey)
    {
        auto root = rootKey.path();
        OpenVault_ entry{};

        if (auto it = openVaults_.find(rootKey); it != openVaults_.end()) {
            root = it.key().path();
            entry = it.value();
            openVaults_.erase(it);
        }

        // ~QObject emits destroyed BEFORE deleting its children, so the
        // window's views may still be alive. Their models are the store's, not
        // this Vault's, so disposing it can't free a buffer under a view. The
        // common Vault is App's, not the window's: it outlives every window
        // onto it, so closing its own window never disposes it
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
            appConfig_.setVaultOpen(root, false);
            appConfig_.save();
        }

        // A close changes derived open-state, so a picker left up re-enables
        // this row's rename / remove-from-list. The entry left openVaults_
        // above, so vaultEntries() already sees this vault as closed
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
    // launch, so persisting each would churn fernanda.json for nothing; it
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
        if (openVaults_.contains(PathKey::of(vaultRoot))) {
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

    // Quit by the usual path, then main starts Fernanda again once this copy is
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
    // hits constantly. Failures are discarded here: the store already logged
    // them, and an autosave-class trigger doesn't prompt. The dirty buffers
    // stay dirty and retry.
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

        flushAll_();
    }

    // The updater's two settings live in fernanda.json, written when either
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
        // each window already flushed the buffers it showed in its closeEvent,
        // so this reduces to any buffer no window showed (a viewless one whose
        // eviction flush failed) and the settings; a still-open window is
        // caught too. The save persists the open set and
        // the last-used timestamps bumped in memory on activation since the
        // last open
        quitting_ = true;

        flushAll_();
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
        flushAll_();
        saveAllWorkspaces_();
        appConfig_.save();
    }
};

} // namespace Fernanda
