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

#include <functional>
#include <utility>

#include <QAbstractButton>
#include <QAction>
#include <QCloseEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPoint>
#include <QSplitter>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/ActionIds.h"
#include "core/AppActions.h"
#include "core/BufferStore.h"
#include "core/PathKey.h"
#include "core/RecentFiles.h"
#include "core/Translations.h"
#include "core/Updater.h"
#include "core/Vault.h"
#include "core/VaultEntry.h"
#include "core/WorkspaceKeys.h"
#include "core/spell/SpellCheckers.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "ui/BaseWindow.h"
#include "ui/FileSwitcher.h"
#include "ui/LossyOpenPrompt.h"
#include "ui/OpenMode.h"
#include "ui/PopoutWindow.h"
#include "ui/UiConstants.h"
#include "ui/WorkspaceFile.h"
#include "ui/settings/SettingsDialog.h"
#include "ui/sidebar/Sidebar.h"
#include "ui/tabs/NewTabPage.h"
#include "ui/tabs/PageIcon.h"
#include "ui/tabs/PagePin.h"
#include "ui/tabs/TabPaneTree.h"
#include "views/AbstractFileView.h"
#include "views/ImageFileView.h"
#include "views/PdfFileView.h"
#include "views/TextFileView.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

class VaultWindow : public BaseWindow
{
    Q_OBJECT

public:
    // Borrows both its own Vault and the Common Vault from App, and App's
    // BufferStore, where every file this window opens is held. commonVault is
    // null when there is no Common Vault, and is the window's own Vault in the
    // Common Vault's own window. It can change later (setCommonVault).
    // vaultsProvider is App's vaultEntries(), passed straight to the Sidebar's
    // switcher. appActions are App's own actions: the base adopts them, and
    // this window hands the set on to every pop-out it makes. spellCheckers is
    // App's too, and is where each text view's dictionary comes from. So are
    // updater and translations, which the settings dialog's General page shows
    VaultWindow(
        Vault* borrowedVault,
        Vault* commonVault,
        BufferStore* bufferStore,
        std::function<QList<VaultEntry>()> vaultsProvider,
        const AppActions& appActions,
        SpellCheckers* spellCheckers,
        Updater* updater,
        Translations* translations)
        : BaseWindow(appActions, updater)
        , vault_(borrowedVault)
        , commonVault_(commonVault)
        , bufferStore_(bufferStore)
        , vaultsProvider_(std::move(vaultsProvider))
        , appActions_(appActions)
        , spellCheckers_(spellCheckers)
        , updater_(updater)
        , translations_(translations)
    {
        setup_();
    }

    ~VaultWindow() override { TRACER; }

    // Sidebar activation lands here. The mode says where the file goes:
    //   ReplaceActive — a double-click or a newly-created file — takes over the
    //     active leaf's current tab (Obsidian's behavior) UNLESS it's pinned,
    //     in which case a new tab; an empty active leaf has no current tab, so
    //     it also just adds.
    //   NewTab — the menu's "Open in new tab" — always adds to the active leaf.
    //   SplitRight — the menu's "Open to the right" — splits the active leaf to
    //     the right and opens in the fresh leaf. Always splits, matching
    //     Obsidian, even off an empty active leaf (which then lingers beside
    //     the file until it's used).
    //   NewWindow — the menu's "Open in new window" — opens in a fresh pop-out.
    // Duping is allowed and we never focus an already-open file, matching
    // Obsidian. The model comes from the vault vaultOf_ names, so a Common
    // Vault file outside this vault resolves to the Common Vault's shared model
    // and syncs across windows for free
    void openFile(const PathKey& key, OpenMode mode)
    {
        auto* view = makeView_(key, lossyOpenPrompt_(this));
        if (!view) {
            return; // refused — unsupported or unreadable; the store warned
        }

        auto title = view->model()->title();

        switch (mode) {
        case OpenMode::NewWindow:
            openInNewWindow_(view, title);
            return;

        case OpenMode::SplitRight: {
            // Split the active leaf to the right — a fresh empty leaf, now
            // active — and drop the view there. splitActiveLeaf changes the
            // active leaf, so this must run before addToActiveLeaf
            auto* tree = tabPaneTree();
            tree->splitActiveLeaf(Qt::Horizontal);
            tree->addToActiveLeaf(view, title);
            return;
        }

        case OpenMode::ReplaceActive:
        case OpenMode::NewTab: {
            auto* tree = tabPaneTree();

            // ReplaceActive takes over the active tab unless pinned — shared
            // with a file dropped on this window's editor; NewTab always adds
            if (mode == OpenMode::ReplaceActive) {
                openReplacingActive_(tree, view, title);
            } else {
                tree->addToActiveLeaf(view, title);
            }
            return;
        }
        }
    }

    // The Common Vault changed, or there's none now (App::setCommonVault_).
    // The sidebar's drawer is replaced or removed, and WorkspaceFile handed the
    // new one. Every view, here and in the pop-outs, follows the new Common
    // Vault's words in place of the old one's and takes its words again, since
    // the Common Vault's dictionary applies here. And every tab is looked at
    // again as if its file had moved: one in neither of this window's vaults
    // now closes, and the rest are marked as guests or not.
    //
    // A view is disconnected from the old Common Vault by name, not every
    // connection from it, and not when it was this window's own vault: then
    // the connection is the view's to its own vault's words, which stays
    void setCommonVault(Vault* commonVault)
    {
        if (commonVault == commonVault_) {
            return;
        }

        auto* old_common = commonVault_;
        auto views = fileViews_();

        if (old_common && old_common != vault_) {
            for (auto* view : views) {
                disconnect(old_common, &Vault::wordsChanged, view, nullptr);
            }
        }

        commonVault_ = commonVault;

        sidebar_->setCommonVault(commonVault_);
        workspaceFile_->setCommonDrawer(
            sidebar_->commonTree(),
            sidebar_->commonDrawer());

        for (auto* view : views) {
            followCommonWords_(view);
            applyWords_(view);
            reconsiderView_(view);
        }
    }

    // App calls this in its exit sweeps for windows that won't get a closeEvent
    // of their own — a multi-window quit or OS logout. Idempotent with the
    // closeEvent save below; flushes any pending workspace debounce
    void saveWorkspaceNow()
    {
        if (workspaceFile_) {
            workspaceFile_->saveNow();
        }
    }

signals:
    // Relayed up from the Sidebar: the user picked another known vault in the
    // switcher. App's convergence point opens or raises it
    void vaultSwitchRequested(const Coco::Path& vaultRoot);

protected:
    void closeEvent(QCloseEvent* event) override
    {
        // Final flush lives here and only here: the buffers this window's views
        // show, in it and its pop-outs. Buffers belong to the BufferStore, not
        // to any window, so a file this window and another both show is one
        // buffer, flushed here; a file only another window shows is left to
        // that window, so a failure there can't hold this one open. It's also
        // the only teardown that can still refuse, so it runs before any: on a
        // failed write we name the files, keep the window open (the buffers
        // stay dirty and keep retrying), and bail before touching the pop-outs
        if (auto failed = bufferStore_->flush(shownKeys_());
            !failed.isEmpty()) {
            // Refuse: these buffers are the only copy, so we keep the window
            // open rather than lose them. That can trap the user if the vault's
            // location is permanently unwritable (drive gone, read-only). The
            // planned escape is a window showing each failed buffer's text to
            // copy out by hand, which needs no disk (docs/Future.md, "Saving
            // and safety")
            //
            // TODO: Reexamine save-failure handling. Obsidian is the general
            // model, but its choices here may not suit: do what is sane and
            // simple, and never lose writing if at all possible
            showSaveFailure_(failed);
            event->ignore();
            return;
        }

        // A settings change a moment old is written now, not left to a timer
        // that may never fire
        vault_->saveSettingsNow();

        // Committed to closing: persist final workspace before tearing anything
        // down. saveNow flushes any pending debounce — the guaranteed capture
        // point for a single-window close, the workspace twin of the buffer
        // flush above. Runs before closePopouts_ so the pop-outs' trees are
        // still alive to serialize
        workspaceFile_->saveNow();

        // Flush done: now close the pop-outs. They're siblings, so Qt won't
        // take them down with us — we do. Files open only in a pop-out were
        // already flushed above. A buffer no window shows (a viewless one
        // whose eviction flush failed) waits for the app-quit sweep
        // (App::onAboutToQuit_)
        closePopouts_();

        event->accept();
    }

private:
    // buildPage_'s return type. lupdate reads a function whose return type is
    // qualified (TabPaneTree::RestoredPage) as a member of the qualifying
    // class, and files its tr() text under that class's name, where the
    // translation is never looked up
    using RestoredPage_ = TabPaneTree::RestoredPage;

    Vault* vault_ = nullptr;
    Vault* commonVault_ = nullptr;
    BufferStore* bufferStore_ = nullptr;

    // App's vaultEntries() — handed to the Sidebar's VaultSwitcher for its
    // "other vaults" menu. Held so setup_ can pass it on
    std::function<QList<VaultEntry>()> vaultsProvider_;

    // App's actions, held only to hand on to pop-outs — the base already
    // adopted them for this window. Not owned here; App outlives every window
    AppActions appActions_{};

    // App's dictionaries, which makeView_ takes each view's from. Not owned
    SpellCheckers* spellCheckers_ = nullptr;

    // App's update checker, for the settings dialog and the status bar's
    // update notice. Not owned
    Updater* updater_ = nullptr;

    // App's interface language, for the settings dialog. Not owned
    Translations* translations_ = nullptr;

    Sidebar* sidebar_ = nullptr;

    // The outer split: sidebar | editor tree. A member (not a setup_ local) so
    // WorkspaceFile can persist and restore its sizes — the sidebar width.
    // Sidebar visibility isn't persisted: there is no toggle to produce it
    QSplitter* splitter_ = nullptr;

    // Machine-local workspace persistence. Owned here, constructed last in
    // setup_, restores before App shows us
    WorkspaceFile* workspaceFile_ = nullptr;

    // The files this window and its pop-outs have made active, newest first,
    // for Go to File. Recorded from every tree's activePageChanged, saved with
    // the workspace
    RecentFiles recentFiles_{};

    // Pop-outs are siblings, not Qt children: a child window can never stack
    // behind its owner. Siblings can — but Qt doesn't reap them when we close,
    // so we own their lifetime here. A pop-out never outlives its VaultWindow
    QList<PopoutWindow*> popouts_{};

    // This vault's settings. Built on first use and kept — hidden, not
    // destroyed, between opens — so it reopens on the page last viewed. A Qt
    // child, so it goes with the window
    SettingsDialog* settingsDialog_ = nullptr;

    void setup_()
    {
        resize(DEFAULT_VAULT_WINDOW_WIDTH, DEFAULT_VAULT_WINDOW_HEIGHT);

        buildActions_();
        buildMenuBar_();

        sidebar_ = new Sidebar(
            vault_,
            commonVault_,
            Sidebar::Actions{
                .manageVaults = action(ActionIds::APP_MANAGE_VAULTS),
                .openSettings = action(ActionIds::VAULT_OPEN_SETTINGS) },
            vaultsProvider_,
            this);

        connect(
            sidebar_,
            &Sidebar::fileActivated,
            this,
            &VaultWindow::openFile);

        connect(
            sidebar_,
            &Sidebar::switchVaultRequested,
            this,
            &VaultWindow::vaultSwitchRequested);

        connect(
            sidebar_,
            &Sidebar::entryRelocated,
            this,
            &VaultWindow::saveWorkspaceNow);

        auto* tree = tabPaneTree();

        // The tree is owned by BaseWindow. It opens as a single empty leaf —
        // underlay showing, no tabs — until workspace restore or the user adds
        // one. A "+" on any leaf routes back here as newTabRequested
        connect(
            tree,
            &TabPaneTree::newTabRequested,
            this,
            &VaultWindow::onNewTabRequested_);

        tree->setFamilyId(this);

        connect(
            tree,
            &TabPaneTree::popOutRequested,
            this,
            &VaultWindow::onPopOutRequested_);

        connect(
            tree,
            &TabPaneTree::fileDropped,
            this,
            &VaultWindow::onFileDropped_);

        connect(
            tree,
            &TabPaneTree::activePageChanged,
            this,
            &VaultWindow::recordRecentFile_);

        // Which dropped files this tree will accept: the ones one of this
        // window's vaults owns. Same resolution onFileDropped_ uses, asked
        // during the drag instead of after the release, so a foreign vault's
        // file refuses visibly. The lambda outlives nothing — a pop-out is
        // closed by this window before it dies
        tree->setEntryOpenFilter([this](const Coco::Path& absolute) {
            return vaultOf_(PathKey::of(absolute)) != nullptr;
        });

        // Sidebar left, editor tree centre. WorkspaceFile persists the split
        // sizes; the values here are the first-run defaults
        splitter_ = new QSplitter(Qt::Horizontal, this);
        splitter_->addWidget(sidebar_);
        splitter_->addWidget(tree);
        splitter_->setStretchFactor(0, 0); // sidebar keeps its width
        splitter_->setStretchFactor(1, 1); // editor takes the slack
        splitter_->setSizes({ 260, 740 });

        setCentralWidget(splitter_);

        // Workspace persistence, constructed last so it can observe this
        // window, its tree, and the sidebar splitter. restore() runs now —
        // before App shows us — so geometry, the reopened files, and the
        // pop-outs land pre-show. The Hooks keep the two things this class
        // can't know here, in the one place that owns them: the path<->view
        // translation (describe/build) and pop-out creation
        workspaceFile_ = new WorkspaceFile(
            this,
            tree,
            splitter_,
            sidebar_->fileTree(),
            sidebar_->commonTree(),
            sidebar_->commonDrawer(),
            vault_->root(),
            WorkspaceFile::Hooks{
                .describePage =
                    [this](QWidget* page) { return describePage_(page); },
                .buildPage =
                    [this](const QJsonValue& value) {
                        return buildPage_(value);
                    },
                .enumeratePopouts = [this] { return enumeratePopouts_(); },
                .createPopout = [this] { return createPopoutForRestore_(); },
                .describeRecentFiles =
                    [this] { return describeRecentFiles_(); },
                .restoreRecentFiles =
                    [this](const QJsonValue& value) {
                        restoreRecentFiles_(value);
                    },
            },
            this);

        workspaceFile_->restore();

        connect(vault_, &Vault::rootRespelled, workspaceFile_, [this] {
            workspaceFile_->setVaultRoot(vault_->root());
        });

        // The status bar's settings (BaseWindow::applyConfig): now, and on
        // every change. Pop-outs get the same in makeWiredPopout_, as they do
        // the update notice's click
        applyConfig(vault_->config());
        connect(vault_, &Vault::configChanged, this, [this] {
            applyConfig(vault_->config());
        });

        connect(
            updateNotice(),
            &QAbstractButton::clicked,
            this,
            &VaultWindow::onUpdateNoticeClicked_);
    }

    // VaultWindow's own actions, added to the registry the base started. The
    // shared document.undo / document.redo and App's adopted app.quit /
    // app.manageVaults already exist (BaseWindow::setup_); these are the ones
    // only a full window has. registerAction window-adds each, so the shortcuts
    // fire whether or not the menu bar is showing — the bar is only a display
    // surface, and a menu-hosted shortcut goes dead while the bar is hidden,
    // but the window-level registration does not
    void buildActions_()
    {
        auto* new_file = registerAction(
            ActionIds::FILE_NEW,
            tr("New File"),
            QKeySequence::New);
        connect(
            new_file,
            &QAction::triggered,
            this,
            &VaultWindow::onNewFileRequested_);

        auto* go_to_file = registerAction(
            ActionIds::FILE_OPEN,
            tr("Go to File..."),
            QKeySequence::Open);
        connect(
            go_to_file,
            &QAction::triggered,
            this,
            &VaultWindow::onGoToFileRequested_);

        auto* new_folder = registerAction(
            ActionIds::FOLDER_NEW,
            tr("New Folder"),
            QKeySequence(u"Ctrl+Shift+N"_s));
        connect(
            new_folder,
            &QAction::triggered,
            this,
            &VaultWindow::onNewFolderRequested_);

        // Ctrl+, is Obsidian's settings hotkey. Explicit rather than
        // QKeySequence::Preferences, which is empty on Windows (as Quit is)
        auto* open_settings = registerAction(
            ActionIds::VAULT_OPEN_SETTINGS,
            tr("Settings..."),
            QKeySequence(u"Ctrl+,"_s));
        connect(
            open_settings,
            &QAction::triggered,
            this,
            &VaultWindow::onSettingsRequested_);

        auto* toggle_menu_bar = registerAction(
            ActionIds::WINDOW_TOGGLE_MENU_BAR,
            tr("Toggle Menu Bar"),
            QKeySequence(u"Ctrl+M"_s));
        connect(
            toggle_menu_bar,
            &QAction::triggered,
            this,
            &VaultWindow::onToggleMenuBar_);
    }

    // The menu bar shows the registry's actions — it doesn't own them. Hidden
    // by default; every action's real shortcut lives on the window
    // (registerAction), so hiding the bar disables nothing, and Ctrl+M brings
    // it back. It always starts hidden; the shown/hidden choice isn't persisted
    void buildMenuBar_()
    {
        auto* bar = menuBar();

        auto* file_menu = bar->addMenu(tr("File"));
        file_menu->addAction(action(ActionIds::FILE_NEW));
        file_menu->addAction(action(ActionIds::FILE_OPEN));
        file_menu->addAction(action(ActionIds::FOLDER_NEW));
        file_menu->addSeparator();
        file_menu->addAction(action(ActionIds::VAULT_OPEN_SETTINGS));
        file_menu->addAction(action(ActionIds::APP_MANAGE_VAULTS));
        file_menu->addSeparator();
        file_menu->addAction(action(ActionIds::APP_QUIT));

        auto* edit_menu = bar->addMenu(tr("Edit"));
        edit_menu->addAction(action(ActionIds::DOCUMENT_UNDO));
        edit_menu->addAction(action(ActionIds::DOCUMENT_REDO));
        edit_menu->addSeparator();
        edit_menu->addAction(action(ActionIds::TEXT_CUT));
        edit_menu->addAction(action(ActionIds::TEXT_COPY));
        edit_menu->addAction(action(ActionIds::TEXT_PASTE));
        edit_menu->addAction(action(ActionIds::TEXT_DELETE));
        edit_menu->addSeparator();
        edit_menu->addAction(action(ActionIds::TEXT_SELECT_ALL));
        edit_menu->addSeparator();
        edit_menu->addAction(action(ActionIds::VIEW_FIND));
        edit_menu->addAction(action(ActionIds::VIEW_REPLACE));
        edit_menu->addAction(action(ActionIds::VIEW_FIND_NEXT));
        edit_menu->addAction(action(ActionIds::VIEW_FIND_PREVIOUS));

        auto* view_menu = bar->addMenu(tr("View"));
        view_menu->addAction(action(ActionIds::WINDOW_TOGGLE_MENU_BAR));
        view_menu->addSeparator();
        view_menu->addAction(action(ActionIds::VIEW_ZOOM_IN));
        view_menu->addAction(action(ActionIds::VIEW_ZOOM_OUT));
        view_menu->addAction(action(ActionIds::VIEW_ZOOM_RESET));

        bar->hide();
    }

    // New File: create an empty file at the vault root and open it over the
    // active leaf's current tab — ReplaceActive, the newly-created-file
    // semantics openFile documents — or add it if that tab is pinned or the
    // leaf is empty. createFile warns and returns empty on failure, which we
    // drop
    void onNewFileRequested_()
    {
        auto key = vault_->createFile(vault_->rootKey());
        if (key.isEmpty()) {
            return;
        }

        openFile(key, OpenMode::ReplaceActive);
    }

    // Go to File: pick with the FileSwitcher over this window and open the way
    // the pick says — Enter replaces the active tab, as a tree double-click
    // does; Ctrl+Enter adds a new one. Rows carry keys from the two vaults'
    // listings, so nothing outside them can be picked. A new file is created
    // first
    void onGoToFileRequested_()
    {
        auto* view = activeFileView();

        FileSwitcher switcher(
            this,
            vault_,
            commonVault_,
            recentFiles_.files(),
            view ? view->model()->key() : PathKey{});
        if (switcher.exec() != QDialog::Accepted) {
            return;
        }

        auto pick = switcher.pick();
        if (!createIfNew_(pick, this)) {
            return;
        }

        openFile(pick.key, pick.mode);
    }

    // A Go to File pick naming a file that isn't there yet is created here,
    // before it opens, in this window's vault: the switcher names new files
    // there. The user typed the name, so a failure is said, over parentWindow
    // (the window the switcher was over). True when the file is there to open
    bool createIfNew_(const FileSwitcher::Pick& pick, QWidget* parentWindow)
    {
        if (!pick.isNew) {
            return true;
        }

        if (vault_->createFileAt(pick.key)) {
            return true;
        }

        QMessageBox::warning(
            parentWindow,
            tr("Couldn't create file"),
            tr("Could not create \"%1\". A file may be in the way, or a "
               "folder may be locked.")
                .arg(displayPathOf_(pick.key).prettyQString()));
        return false;
    }

    void onSettingsRequested_() { showSettings_(); }

    // The status bar's update notice, here or in a pop-out: the notice goes
    // from every window, and this vault's settings open on the General page,
    // where the update is
    void onUpdateNoticeClicked_()
    {
        updater_->dismiss();
        showSettings_(true);
    }

    // Show this vault's settings dialog, window-modal. open(), never exec() —
    // see SettingsDialog for why a nested event loop here is unsafe. Built on
    // first use; after that the same dialog is shown again, still on the page
    // it was left on unless onGeneralPage asks for General. The quit action is
    // handed in so Ctrl+Q works while the dialog has focus
    void showSettings_(bool onGeneralPage = false)
    {
        if (!settingsDialog_) {
            settingsDialog_ = new SettingsDialog(
                vault_,
                spellCheckers_,
                updater_,
                translations_,
                action(ActionIds::APP_QUIT),
                action(ActionIds::APP_RESTART),
                this);
        }

        if (onGeneralPage) {
            settingsDialog_->showGeneralPage();
        }

        settingsDialog_->open();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    }

    // New Folder: create an empty folder at the vault root. No view to open — a
    // folder isn't a file — so there's nothing more to do; the sidebar tree
    // reflects it exactly as the tree's own New Folder does. createFolder warns
    // and no-ops on failure
    void onNewFolderRequested_() { vault_->createFolder(vault_->rootKey()); }

    // Ctrl+M / View menu: flip the menu bar's visibility. The shortcut is
    // window-owned, so it works while the bar is hidden — the way back from a
    // hidden bar.
    //
    // Windows and Linux only, by design. macOS merges a QMainWindow's menu bar
    // into the system bar, which is not ours to hide, and Ctrl+M maps to Cmd+M
    // there — Minimize. Hiding-by-default and this toggle are both no-ops (or
    // worse) on that platform; if macOS ships, this needs a platform gate, not
    // a fix. Ctrl+M itself can be rebound any time it proves awkward on the
    // platforms it does serve
    void onToggleMenuBar_()
    {
        auto* bar = menuBar();
        bar->setVisible(!bar->isVisible());
    }

    // Build a fresh view on the file's (deduped) model, choosing the view by
    // the model's concrete type. The model is the BufferStore's; openModel
    // dedups it, so opening the same file twice — here, in a pop-out, or in
    // another window — shares one buffer and one document/prime. Returns the
    // base type — callers work through it, and the one text-only touch
    // (cursor/scroll restore) casts down in buildPage_. Returns nullptr for a
    // file in neither of this window's vaults, and when the store refuses the
    // file (an unsupported type, an unreadable file, or a non-UTF-8 one the
    // caller's confirmLossyOpen declined) — every caller early-returns on it
    [[nodiscard]] AbstractFileView* makeView_(
        const PathKey& key,
        const BufferStore::ConfirmLossyOpen& confirmLossyOpen)
    {
        if (!vaultOf_(key)) {
            WARN(
                "Refusing to open {}: in neither of this window's vaults!",
                key.path());
            return nullptr;
        }

        auto* model = bufferStore_->openModel(key, confirmLossyOpen);
        if (!model) {
            return nullptr;
        }

        AbstractFileView* view = nullptr;

        if (auto* text = qobject_cast<TextFileModel*>(model)) {
            auto* text_view = new TextFileView(text);

            // Words go to this window's vault, whichever vault the file is in
            connect(
                text_view,
                &TextFileView::addToDictionaryRequested,
                vault_,
                &Vault::addToDictionary);
            connect(
                text_view,
                &TextFileView::ignoreWordRequested,
                vault_,
                &Vault::ignoreWord);

            view = text_view;
        } else if (auto* pdf = qobject_cast<PdfFileModel*>(model)) {
            view = new PdfFileView(pdf);
        } else if (auto* image = qobject_cast<ImageFileModel*>(model)) {
            view = new ImageFileView(image);
        }

        if (!view) {
            ASSERT(false, "Unknown model type in makeView_!");
            return nullptr;
        }

        // Settings come from THIS window's vault, not the file's: a Common
        // Vault file open here looks like its neighbours. Applied now, and
        // again on every change. The view is the connection's context, so the
        // link dies with it; the vault is captured by value because it outlives
        // every view this window (or its pop-outs) will ever host.
        //
        // The dictionary is one of those settings, but not something a view
        // can make from the config alone, so it is found here and handed over:
        // none while spellcheck is off, or when the language has no dictionary
        auto* vault = vault_;
        auto apply = [view, vault, spell_checkers = spellCheckers_] {
            const auto& config = vault->config();
            view->applyConfig(config);
            view->setSpellChecker(
                config.spellcheck()
                    ? spell_checkers->checker(config.spellcheckLanguage())
                    : nullptr);
        };

        apply();
        connect(vault, &Vault::configChanged, view, apply);

        // The words to take as correct (applyWords_), taken now and again when
        // either vault's words change. The view dies before this window
        // (pop-outs close with it), so capturing this is safe
        applyWords_(view);

        connect(vault, &Vault::wordsChanged, view, [this, view] {
            applyWords_(view);
        });

        followCommonWords_(view);

        // A rename or move, from this window or another, may take the file
        // out of this window's vaults or into the other of them. Looked at a
        // tick later, once App has seen the move through: a move of the Common
        // Vault's folder makes a new Common Vault there
        // (App::followCommonVault_), and its files are still a Common Vault's
        connect(model, &AbstractFileModel::renamed, view, [this, view] {
            Coco::Time::onNextTick(view, [this, view] {
                onViewFileRelocated_(view);
            });
        });

        applyGuestMark_(view, key);
        return view;
    }

    // The confirmation behind every user-initiated open. parent is the window
    // the user is looking at — this one, or the pop-out whose tree the open
    // came from — so the warning lands over it rather than over a main window
    // that may be behind or elsewhere
    [[nodiscard]] BufferStore::ConfirmLossyOpen
    lossyOpenPrompt_(QWidget* parent) const
    {
        return [this, parent](const PathKey& key, const QByteArray& data) {
            return LossyOpenPrompt::confirm(displayPathOf_(key), data, parent);
        };
    }

    // Mark a guest tab, or clear the mark: a guest is a file outside this
    // window's own vault, which means a Common Vault file open in a project
    // window that doesn't contain it. A Common Vault file inside this vault is
    // this vault's, and in the Common Vault's own window every file is, so
    // nothing is marked there. The mark rides the page (PageIcon), so it
    // survives a drag into a pop-out and a workspace restore. It changes only
    // when a rename or move takes the file between this window's two vaults,
    // or the Common Vault changes (reconsiderView_). The leaf renders the
    // glyph in its own colors. True when the mark changed
    bool applyGuestMark_(AbstractFileView* view, const PathKey& key)
    {
        auto path = vault_->contains(key)
                        ? QString{}
                        : QString::fromLatin1(COMMON_VAULT_ICON_PATH);

        if (pageIconPath(view) == path) {
            return false;
        }

        setPageIconPath(view, path);
        return true;
    }

    // A view's file was renamed or moved, by this window or by another: vaults
    // nest, so another vault's window can move a file this window shows.
    // workspace.json records the old path, so a save is scheduled
    void onViewFileRelocated_(AbstractFileView* view)
    {
        reconsiderView_(view);

        if (workspaceFile_) {
            workspaceFile_->scheduleSave();
        }
    }

    // Which of this window's vaults a view's file is in may have changed: its
    // file moved, or the Common Vault did. In neither, the view closes; the
    // buffer lives on in any window that still shows it, or, if none does, its
    // last view's save writes any unsaved text to the file. Otherwise its
    // guest mark follows the vault it's in now
    void reconsiderView_(AbstractFileView* view)
    {
        auto key = view->model()->key();

        if (!vaultOf_(key)) {
            view->deleteLater();
            return;
        }

        if (applyGuestMark_(view, key)) {
            if (auto* tree = treeHosting_(view)) {
                tree->refreshPageIcon(view);
            }
        }
    }

    // The words a view takes as correct: this window's vault's dictionary and
    // ignored words, and the Common Vault's dictionary, if there is one, which
    // applies in every vault. Read when called, since the Common Vault can
    // change
    void applyWords_(AbstractFileView* view) const
    {
        auto words = vault_->dictionary();
        words.unite(vault_->ignoredWords());

        if (commonVault_ && commonVault_ != vault_) {
            words.unite(commonVault_->dictionary());
        }

        view->setAcceptedWords(words);
    }

    // The view takes its words again when the Common Vault's words change.
    // Nothing
    // when there's no Common Vault, or this window is it: then its words are
    // this vault's, which the view already follows
    void followCommonWords_(AbstractFileView* view)
    {
        if (commonVault_ && commonVault_ != vault_) {
            connect(commonVault_, &Vault::wordsChanged, view, [this, view] {
                applyWords_(view);
            });
        }
    }

    // Open view into tree's active leaf, taking over the current tab unless
    // it's pinned or the leaf is empty (Obsidian's replace-active). Shared by
    // the sidebar/menu ReplaceActive open on this window's tree and a file
    // dropped on an editor — this window's tree or a pop-out's. The dropped-on
    // leaf is made active by the tree before it signals, so "active leaf" there
    // is the leaf the file landed on
    void
    openReplacingActive_(TabPaneTree* tree, QWidget* view, const QString& title)
    {
        auto* leaf = tree->activeLeaf();
        auto* current = leaf ? leaf->currentPage() : nullptr;

        if (current && !isPagePinned(current)) {
            tree->replacePage(current, view, title);
        } else {
            tree->addToActiveLeaf(view, title);
        }
    }

    // The file views in this window and in its pop-outs. A pop-out has no
    // parent, so it isn't among this window's children and is searched on its
    // own
    [[nodiscard]] QList<AbstractFileView*> fileViews_() const
    {
        auto views = findChildren<AbstractFileView*>();

        for (auto* popout : popouts_) {
            views << popout->findChildren<AbstractFileView*>();
        }

        return views;
    }

    // The files this window's views show, in it and in its pop-outs. A file
    // shown twice is listed twice
    [[nodiscard]] QList<PathKey> shownKeys_() const
    {
        QList<PathKey> keys{};

        for (auto* view : fileViews_()) {
            keys << view->model()->key();
        }

        return keys;
    }

    // Which of this window's vaults a file belongs to: this window's own for
    // any file inside its root, even where the vaults nest and the Common
    // Vault lists it too; else the Common Vault for a file inside that; else
    // none. Opening, the guest mark, and workspace.json all go by this, so in
    // one window a file belongs to one vault
    [[nodiscard]] Vault* vaultOf_(const PathKey& key) const
    {
        if (vault_->contains(key)) {
            return vault_;
        }

        if (commonVault_ && commonVault_->contains(key)) {
            return commonVault_;
        }

        return nullptr;
    }

    // A file's path as this window's trees show it: relative to the root of
    // the vault it belongs to here. The whole path for a file in neither
    [[nodiscard]] Coco::Path displayPathOf_(const PathKey& key) const
    {
        auto* owner = vaultOf_(key);
        return owner ? key.relativeTo(owner->rootKey()) : key.path();
    }

    // A file as workspace.json names it: "vault" (this/common, from this
    // window's point of view) and "file", a path relative to that vault's
    // root. False, writing nothing, for a file in neither of this window's
    // vaults. In the Common Vault's own window the two vaults are one object
    // (App reuses its common Vault), so every file there is written as this
    // window's; both labels resolve to the same Vault on restore in that window
    bool describeFile_(const PathKey& key, QJsonObject& obj) const
    {
        namespace WK = WorkspaceKeys;

        auto* owner = vaultOf_(key);
        if (!owner) {
            return false;
        }

        obj[WK::VAULT] = (owner == vault_) ? WK::VAULT_THIS : WK::VAULT_COMMON;
        obj[WK::FILE_PATH] = key.relativeTo(owner->rootKey()).prettyQString();
        return true;
    }

    // The reverse of describeFile_. Empty for a vault this window doesn't have,
    // or a path that isn't a plain relative one, so a hand-edited entry can't
    // name a file outside its vault
    [[nodiscard]] PathKey keyFromWorkspace_(const QJsonObject& obj) const
    {
        namespace WK = WorkspaceKeys;

        auto relative = Coco::Path(obj.value(WK::FILE_PATH).toString());
        auto* vault = (obj.value(WK::VAULT).toString() == WK::VAULT_COMMON)
                          ? commonVault_
                          : vault_;

        if (!vault || relative.isEmpty() || !relative.isPlain() ||
            relative.hasRoot()) {
            return {};
        }

        return PathKey::of(vault->root() / relative);
    }

    // Persist one page as a workspace.json tab entry, or drop it. File views
    // and empty tabs (NewTabPage) both persist; only an unknown page type
    // returns a null value the tree omits. The envelope (type/vault/file/
    // pinned) is owned here; per-view viewport state nests under "state" as an
    // opaque blob this method never reads into — the view owns its schema
    // (Obsidian's state shape). The file is written by describeFile_
    [[nodiscard]] QJsonValue describePage_(QWidget* page) const
    {
        namespace WK = WorkspaceKeys;

        // An empty tab persists as a typed entry so buildPage_ can tell it from
        // a file; only its pin state rides along
        if (qobject_cast<NewTabPage*>(page)) {
            QJsonObject obj{};
            obj[WK::TYPE] = WK::TYPE_NEWTAB;
            obj[WK::PINNED] = isPagePinned(page);
            return obj;
        }

        auto* view = qobject_cast<AbstractFileView*>(page);
        if (!view) {
            return QJsonValue{}; // an unknown page type — still dropped
        }

        QJsonObject obj{};
        if (!describeFile_(view->model()->key(), obj)) {
            return QJsonValue{};
        }

        obj[WK::PINNED] = isPagePinned(page);

        // The view fills its own state; omit the key when it writes nothing
        QJsonObject state{};
        view->writeViewState(state);
        if (!state.isEmpty()) {
            obj[WK::STATE] = state;
        }

        return obj;
    }

    // A page that becomes a tree's active page is the file it shows, when it
    // shows one; a new tab or an empty pane records nothing
    void recordRecentFile_(QWidget* page)
    {
        if (auto* view = qobject_cast<AbstractFileView*>(page)) {
            recentFiles_.record(view->model()->key());
        }
    }

    // The recent files as workspace.json keeps them, each written as
    // describeFile_ writes a tab's file. A file no longer on disk is left out,
    // so a deleted or moved file drops out of the list at the next save
    [[nodiscard]] QJsonValue describeRecentFiles_() const
    {
        QJsonArray array{};

        for (const auto& key : recentFiles_.files()) {
            if (!key.path().exists()) {
                continue;
            }

            QJsonObject obj{};
            if (describeFile_(key, obj)) {
                array.append(obj);
            }
        }

        return array;
    }

    // The saved list replaces whatever restoring the tabs recorded. A
    // workspace saved before there was a list has none, and keeps those. An
    // entry keyFromWorkspace_ can't read is left out
    void restoreRecentFiles_(const QJsonValue& value)
    {
        if (!value.isArray()) {
            return;
        }

        QList<PathKey> files{};

        for (const auto& entry : value.toArray()) {
            if (auto key = keyFromWorkspace_(entry.toObject());
                !key.isEmpty()) {
                files << key;
            }
        }

        recentFiles_.setFiles(files);
    }

    // Rebuild a persisted tab into a live view, or drop it. Resolves the vault
    // (this/common), checks the file still exists — a moved/deleted file is
    // silently dropped — then mints a view on the (deduped) model exactly as
    // openFile does and hands it its own state blob to restore. Non-const:
    // makeView_ opens a model, mutating the store's map
    [[nodiscard]] RestoredPage_ buildPage_(const QJsonValue& value)
    {
        namespace WK = WorkspaceKeys;

        auto obj = value.toObject();

        // A persisted empty tab. Rebuild and wire it exactly like the "+" path;
        // its buttons resolve their host tree at click time
        if (obj.value(WK::TYPE).toString() == WK::TYPE_NEWTAB) {
            auto* page = new NewTabPage;
            wireNewTabPage_(page);
            setPagePinned(page, obj.value(WK::PINNED).toBool());
            return { page, tr("New tab") };
        }

        auto key = keyFromWorkspace_(obj);
        if (key.isEmpty()) {
            return {};
        }

        // Missing file → drop the view silently. openModel refuses a missing
        // file itself, so this check is advisory — kept for its specific DEBUG
        // line, which says WHY a tab fell out of the workspace where
        // openModel's refusal is a generic WARN
        if (!key.path().exists()) {
            DEBUG(
                this,
                "Workspace restore: dropping missing file {}",
                key.path());
            return {};
        }

        // An unsupported, unreadable, or non-UTF-8 file is dropped the same
        // way, a file another program holds locked at launch included. A
        // non-UTF-8 file is refused rather than prompted for: restore runs
        // inside the constructor, before show(), and a modal prompt there would
        // spin a nested event loop into a half-built window. It rarely bites —
        // an accepted lossy open is saved as UTF-8 at once, so only a file
        // re-encoded between sessions lands here. Either way the tab is gone
        // for good, as for a missing file
        auto* view =
            makeView_(key, [this](const PathKey& dropped, const QByteArray&) {
                DEBUG(
                    this,
                    "Workspace restore: dropping non-UTF-8 file {}",
                    dropped.path());
                return false;
            });
        if (!view) {
            return {};
        }

        // Restore per-view viewport state from the opaque blob — the view owns
        // its schema and defers whatever needs layout (a text scroll, a PDF
        // page) to its own showEvent. An absent blob restores at defaults
        view->readViewState(obj.value(WK::STATE).toObject());

        setPagePinned(view, obj.value(WK::PINNED).toBool());

        return { view, view->model()->title() };
    }

    // The new-tab page becomes the editor in place: same tab, page swapped, in
    // whichever tree the "+" came from
    void openInto_(TabPaneTree* tree, NewTabPage* page, const PathKey& key)
    {
        auto* view = makeView_(key, lossyOpenPrompt_(tree->window()));
        if (!view) {
            return; // refused (unsupported or unreadable) — the new tab page
                    // stays as it was
        }

        tree->replacePage(page, view, view->model()->title());
    }

    // The one save-failure prompt that survives. Autosave-class triggers —
    // debounce, ceiling, focus-loss, quit — only log and retry; a window close
    // is the sole place a failure is visible and can still refuse, so the
    // message says as much. Names the files (vault-relative, as the tree shows
    // them) and offers only OK: the window stays open, and the dirty buffers
    // keep retrying on every autosave tick and on the next close attempt
    void showSaveFailure_(const QList<PathKey>& failed)
    {
        QStringList names{};
        for (const auto& key : failed) {
            names << displayPathOf_(key).prettyQString();
        }

        auto bullet = u"\n\u2022 "_s;
        auto list = bullet + names.join(bullet);

        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(tr("Couldn't save"));
        box.setText(
            tr("Fernanda couldn't save these files:%1\n\nThe window will stay "
               "open so you don't lose your changes. Once the problem is "
               "resolved (for example, freeing disk space or reconnecting a "
               "drive), close again to retry.")
                .arg(list));
        box.setTextInteractionFlags(Qt::TextSelectableByMouse);
        box.setWindowModality(Qt::WindowModal);

        auto* ok = box.addButton(QMessageBox::Ok);
        box.setDefaultButton(ok);
        box.setEscapeButton(ok);
        box.exec();
    }

    // Track a freshly-made pop-out and bind its teardown. WA_DeleteOnClose
    // (from BaseWindow) reaps a user-closed pop-out; its destroyed prunes the
    // list. removeOne is pure pointer identity — safe to run mid-destruction,
    // since we never dereference. Same pattern App uses for openVaults_
    void addPopout_(PopoutWindow* popout)
    {
        popouts_.append(popout);

        connect(popout, &QObject::destroyed, this, [this, popout] {
            popouts_.removeOne(popout);
        });
    }

    // Closing the VaultWindow closes its pop-outs. deleteLater, not close(): a
    // pop-out has no close logic and must not gain any, so we skip its
    // closeEvent entirely. The vault-scope flush in closeEvent already caught
    // every buffer — pop-out files and main-window files alike — so there's
    // nothing left for a pop-out to save on the way out. The destroyed handler
    // prunes popouts_ as each deferred delete lands; iterate a copy so that
    // can't bite us
    void closePopouts_()
    {
        const auto popouts = popouts_;
        for (auto* popout : popouts) {
            popout->deleteLater();
        }
    }

    // The one place a pop-out is wired: tracked for teardown, family-stamped,
    // its tree's new-tab / pop-out requests routed back here, and auto-closed
    // when it empties. Also handed to WorkspaceFile to persist. Left hidden —
    // callers show it (drag path) or let WorkspaceFile decide (restore)
    PopoutWindow* makeWiredPopout_()
    {
        auto* popout = new PopoutWindow(appActions_, updater_);
        addPopout_(popout);

        auto* tree = popout->tabPaneTree();
        tree->setFamilyId(this);

        connect(
            tree,
            &TabPaneTree::newTabRequested,
            this,
            &VaultWindow::onNewTabRequested_);

        connect(
            tree,
            &TabPaneTree::popOutRequested,
            this,
            &VaultWindow::onPopOutRequested_);

        connect(
            tree,
            &TabPaneTree::fileDropped,
            this,
            &VaultWindow::onFileDropped_);

        connect(
            tree,
            &TabPaneTree::activePageChanged,
            this,
            &VaultWindow::recordRecentFile_);

        // Which dropped files this tree will accept: the ones one of this
        // window's vaults owns. Same resolution onFileDropped_ uses, asked
        // during the drag instead of after the release, so a foreign vault's
        // file refuses visibly. The lambda outlives nothing — a pop-out is
        // closed by this window before it dies
        tree->setEntryOpenFilter([this](const Coco::Path& absolute) {
            return vaultOf_(PathKey::of(absolute)) != nullptr;
        });

        // Its last tab closing leaves the pop-out with nothing to show, so
        // close it. close() runs the ordinary WA_DeleteOnClose path — same as
        // the user closing it by hand, and destroyed prunes popouts_ — so the
        // pop-out still needs no close logic of its own
        connect(tree, &TabPaneTree::emptied, popout, &QWidget::close);

        // The status bar's settings come from this window's vault, as its
        // views' do (makeView_). The pop-out is the connection's context, so
        // the link dies with it; the vault outlives every pop-out
        auto* vault = vault_;
        popout->applyConfig(vault->config());
        connect(vault, &Vault::configChanged, popout, [popout, vault] {
            popout->applyConfig(vault->config());
        });

        connect(
            popout->updateNotice(),
            &QAbstractButton::clicked,
            this,
            &VaultWindow::onUpdateNoticeClicked_);

        // Persist this pop-out too: its geometry and tab layout ride in
        // workspace.json alongside the main window. Guarded because a pop-out
        // could in principle be made before workspaceFile_ exists (it never is
        // — both callers run after setup_ builds it)
        if (workspaceFile_) {
            workspaceFile_->observePopout(popout, tree);
        }

        return popout;
    }

    // Workspace restore asks for a pop-out to fill. Same wiring as a
    // drag-created one, but left hidden — WorkspaceFile restores its geometry
    // and tree, then shows it (or discards it if every file it held is gone)
    WorkspaceFile::PopoutHandle createPopoutForRestore_()
    {
        auto* popout = makeWiredPopout_();
        return { popout, popout->tabPaneTree() };
    }

    // Every live pop-out, for WorkspaceFile to serialize
    QList<WorkspaceFile::PopoutHandle> enumeratePopouts_() const
    {
        QList<WorkspaceFile::PopoutHandle> handles{};
        handles.reserve(popouts_.size());

        for (auto* popout : popouts_) {
            handles.append({ popout, popout->tabPaneTree() });
        }

        return handles;
    }

    // Which of this family's trees currently holds page — the main tree or a
    // pop-out's. A NewTabPage's buttons call this at click time instead of
    // capturing a tree, so they act on wherever the page lives now: right after
    // a restore (buildPage_ has no tree to capture) and after a drag into a
    // pop-out (a captured tree would be stale). Pointer identity only
    [[nodiscard]] TabPaneTree* treeHosting_(QWidget* page) const
    {
        if (auto* main = tabPaneTree(); main->containsPage(page)) {
            return main;
        }

        for (auto* popout : popouts_) {
            if (auto* tree = popout->tabPaneTree(); tree->containsPage(page)) {
                return tree;
            }
        }

        return nullptr;
    }

    // Wire a NewTabPage's three intents, shared by the "+" path and restore.
    // The captured tree is deliberately absent — each intent resolves its host
    // tree via treeHosting_ when it fires, so a restored or dragged page always
    // acts on the tree holding it now. A page somehow in no tree just no-ops
    void wireNewTabPage_(NewTabPage* page)
    {
        connect(page, &NewTabPage::createRequested, this, [this, page] {
            auto* tree = treeHosting_(page);
            if (!tree) {
                return;
            }

            auto key = vault_->createFile(vault_->rootKey());
            if (key.isEmpty()) {
                return; // Vault already warned
            }

            openInto_(tree, page, key);
        });

        // Go to file from an empty tab. The switcher opens over the window
        // hosting the page — a pop-out's page gets it over the pop-out, not
        // this window — and the pick fills the page whatever its mode: the page
        // exists to be filled. The host tree is resolved AFTER exec, not
        // before, so the lookup reflects where the page lives once the modal
        // loop returns. A new file is created first
        connect(page, &NewTabPage::openRequested, this, [this, page] {
            FileSwitcher switcher(
                page->window(),
                vault_,
                commonVault_,
                recentFiles_.files(),
                PathKey{});
            if (switcher.exec() != QDialog::Accepted) {
                return;
            }

            auto* tree = treeHosting_(page);
            if (!tree) {
                return;
            }

            auto pick = switcher.pick();
            if (!createIfNew_(pick, page->window())) {
                return;
            }

            openInto_(tree, page, pick.key);
        });

        connect(page, &NewTabPage::closeRequested, this, [this, page] {
            if (auto* tree = treeHosting_(page)) {
                tree->closePage(page);
            }
        });
    }

    void onNewTabRequested_()
    {
        // A pop-out's tree emits this too, so act on the tree that asked
        auto* tree = qobject_cast<TabPaneTree*>(sender());
        if (!tree) {
            tree = tabPaneTree();
        }

        auto* page = new NewTabPage;
        wireNewTabPage_(page);
        tree->addToActiveLeaf(page, tr("New tab"));
    }

    // A tab was dragged somewhere invalid. Make a pop-out in this family, host
    // the (already-detached) page in it, and show it at the cursor. All the
    // wiring lives in makeWiredPopout_, shared with the restore path
    void onPopOutRequested_(
        QWidget* page,
        const QString& title,
        const QPoint& globalPos)
    {
        auto* popout = makeWiredPopout_();
        popout->tabPaneTree()->addToActiveLeaf(page, title);

        popout->move(globalPos);
        popout->show();
        popout->raise();
        popout->activateWindow();
    }

    // A file was dropped on an editor pane to open it. The emitting tree — this
    // window's or a pop-out's — has already made the dropped-on leaf active, so
    // opening into its active leaf lands the view where the user aimed. A file
    // under neither this vault nor the common one (a drag from another window's
    // vault) isn't ours to open, so it's dropped silently — the same non-event
    // as a tab dropped into a foreign family
    void onFileDropped_(const Coco::Path& absolute)
    {
        auto* tree = qobject_cast<TabPaneTree*>(sender());
        if (!tree) {
            return;
        }

        auto key = PathKey::of(absolute);
        if (!vaultOf_(key)) {
            return;
        }

        auto* view = makeView_(key, lossyOpenPrompt_(tree->window()));
        if (!view) {
            return; // refused (unsupported or unreadable)
        }

        openReplacingActive_(tree, view, view->model()->title());
    }

    // "Open in new window": a fresh pop-out in this family, the view added to
    // its (empty) active leaf. makeWiredPopout_ already handles the family
    // stamp, request routing, persistence, and auto-close-when-emptied — same
    // as a drag-born pop-out. The difference is placement: a drag drops at the
    // cursor (onPopOutRequested_), but a menu open has no such point, so we
    // cascade it off this window rather than land it under the menu
    void openInNewWindow_(QWidget* view, const QString& title)
    {
        auto* popout = makeWiredPopout_();
        popout->tabPaneTree()->addToActiveLeaf(view, title);

        popout->move(frameGeometry().topLeft() + QPoint(48, 48));
        popout->show();
        popout->raise();
        popout->activateWindow();
    }
};

} // namespace Fernanda::Ui
