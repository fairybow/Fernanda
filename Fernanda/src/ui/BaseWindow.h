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
#include <QChar>
#include <QEvent>
#include <QHash>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMoveEvent>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStatusBar>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/ActionIds.h"
#include "core/AppActions.h"
#include "core/Updater.h"
#include "core/VaultConfig.h"
#include "models/AbstractFileModel.h"
#include "ui/CursorPosition.h"
#include "ui/UpdateNotice.h"
#include "ui/WordCounter.h"
#include "ui/tabs/TabPaneTree.h"
#include "views/AbstractFileView.h"
#include "views/TextFileView.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The shared base for both window types. Owns the TabPaneTree and window-wide
// chrome; subclasses arrange the tree into their central widget — PopoutWindow
// sets it directly, VaultWindow puts it beside the Sidebar.
//
// Also home to the per-window action registry. The registry itself is PRIVATE,
// so a subclass cannot file an action behind its back; there are exactly three
// ways in and one way out:
//
//   registerAction(id, text, shortcut)      — mint, window-add, file
//   registerAction(id, text, StandardKey)   — same, platform bindings
//   adoptAction(id, action)                 — window-add and file an action
//                                             this window did not create
//   action(id)                              — read, asserting on a miss
//
// Every registered action is added to the window, so its shortcut fires
// whenever the window is active, independent of any menu's visibility. Menus
// (VaultWindow only) are display surfaces that addAction the pointers; a
// PopoutWindow has no menu and relies on the same actions purely for their
// shortcuts.
//
// Two sources fill the registry. The window MINTS what only it can perform:
// document.undo / document.redo dispatch through this window's own active view
// (below), so each window needs its own. It ADOPTS what App performs — every
// pointer in AppActions — since those must behave identically from any window
// and App connects them once at mint.
//
// Ids name the thing acted on, never the menu the action currently sits in — a
// menu can move, and these strings are user-visible in a future command palette
// and persisted in the hotkey config. "document." is the buffer, not the tab or
// the editor widget: undo routes through the model's shared prime, so it
// reverses in every view of that file, in every window.
//
// And home to the status bar, so both window types have one. Its items report
// on this window's active page (TabPaneTree::activePageChanged) when that page
// is a text view, and hide when it isn't — over a PDF, an image, a New Tab
// page, or an empty pane the bar stays up, empty. It doesn't come and go with
// the page type, which would change the height of everything above it on
// each tab switch. The update notice (UpdateNotice) sits at the left of the
// items, and keeps the bar up while it shows
class BaseWindow : public QMainWindow
{
    Q_OBJECT

public:
    BaseWindow(const AppActions& appActions, Updater* updater)
        : QMainWindow(nullptr)
        , updater_(updater)
    {
        setup_(appActions);
    }

    virtual ~BaseWindow() override {}

    // Both window types host exactly one tree. Public so VaultWindow can
    // configure the trees of the pop-outs it creates
    [[nodiscard]] TabPaneTree* tabPaneTree() const noexcept { return tree_; }

    // The status bar's update notice. Public so VaultWindow can wire its
    // click, here and in each pop-out it makes
    [[nodiscard]] UpdateNotice* updateNotice() const noexcept
    {
        return updateNotice_;
    }

    // The status bar's settings, from the hosting vault's config. A window
    // doesn't know its vault — a pop-out has none of its own — so the
    // VaultWindow calls this on itself and on each pop-out it makes, at
    // construction and on every Vault::configChanged; the same arrangement, and
    // the same hosting-vault rule, as the views' applyConfig. With both items
    // switched off the bar itself hides, giving its strip back to the editor,
    // unless the update notice is showing. With either on it stays, even over
    // a page that leaves it empty (see class note)
    void applyConfig(const VaultConfig& config)
    {
        wordCounter_->applyConfig(config);
        cursorPosition_->applyConfig(config);

        itemsEnabled_ =
            config.wordCounterEnabled() || config.cursorPositionEnabled();
        updateStatusBar_();
    }

signals:
    // This window became the active (frontmost) window. App listens on
    // VaultWindows to track the most-recent vault; PopoutWindow inherits the
    // signal but nothing consumes it yet. Gained-activation only — the
    // isActiveWindow() guard drops the matching deactivation
    void windowActivated();

    // The window moved or resized. WorkspaceFile debounces a workspace.json
    // write on this. Fires rapidly during a drag — which is exactly what the
    // debounce is for. Lives on the base so pop-outs reuse it
    void geometryChanged();

protected:
    // Mint an action into the registry: owned by the window, added to the
    // window so its shortcut is live whenever the window is active (menu shown
    // or not), and filed under a stable id. Returns the pointer for a caller to
    // connect and for a menu to display. Two entry points differ only in the
    // shortcut. The StandardKey form expands to every platform binding — Redo
    // is Ctrl+Y and Ctrl+Shift+Z — so the action stays in step with
    // TextFileView's matches(QKeySequence::Redo) yield. The explicit
    // QKeySequence form is for our own bindings (Ctrl+M); pass an empty
    // QKeySequence for a shortcut-less action
    QAction* registerAction(
        const QString& id,
        const QString& text,
        const QKeySequence& shortcut)
    {
        auto* action = makeAction_(id, text);
        action->setShortcut(shortcut);
        return action;
    }

    QAction* registerAction(
        const QString& id,
        const QString& text,
        QKeySequence::StandardKey key)
    {
        auto* action = makeAction_(id, text);
        action->setShortcuts(key);
        return action;
    }

    // Multiple explicit sequences on one action — for a binding a StandardKey
    // doesn't express and that wants more than one key (zoom-in on Ctrl+= AND
    // Ctrl++, so the no-shift '=' works on a US layout). This is the plural-
    // sequence form the rebindable-hotkey note below is written around
    QAction* registerAction(
        const QString& id,
        const QString& text,
        const QList<QKeySequence>& shortcuts)
    {
        auto* action = makeAction_(id, text);
        action->setShortcuts(shortcuts);
        return action;
    }

    // File an action this window did not create — the AppActions set, and
    // anything else App or another owner hands down later. Window-added like a
    // minted one, so a shortcut given to it later is live without revisiting
    // this, and so the registry's invariant holds for every entry: if it's in
    // here, it's on the window. The borrowing path is deliberately narrow — a
    // subclass cannot reach the hash any other way
    void adoptAction(const QString& id, QAction* action)
    {
        ASSERT(action, "adoptAction requires an action!");

        addAction(action);
        fileAction_(id, action);
    }

    // Read an action out of the registry. Asserts on a miss, so a mistyped or
    // not-yet-registered id stops a debug build at the call site rather than
    // vanishing. In a release build this returns nullptr and the caller's
    // QMenu::addAction warns and skips the item — a silent gap, so the assert
    // is the guard, not the type
    [[nodiscard]] QAction* action(const QString& id) const
    {
        auto* found = actionRegistry_.value(id);
        ASSERT(found, "No action is registered under id: {}!", id);

        return found;
    }

    // The active view of THIS window's tree — the current page of the active
    // leaf when it's a file view, else nullptr (an empty leaf, or a
    // NewTabPage). The active leaf follows focus, so with splits this is the
    // pane being worked in. The window's view.zoom* actions dispatch through
    // this; a view that can't zoom no-ops via AbstractFileView's defaults.
    // Pointer identity only — the cast down (e.g. to a text or PDF view)
    // happens at the use site
    [[nodiscard]] AbstractFileView* activeFileView() const
    {
        auto* leaf = tree_->activeLeaf();
        return leaf ? qobject_cast<AbstractFileView*>(leaf->currentPage())
                    : nullptr;
    }

    // The file model backing the active view, or nullptr when the active leaf
    // is empty or its current page is not a file view. The window's Undo/Redo
    // dispatch through this; a non-editable model (image/PDF) or none at all
    // yields an inert undo via the base's no-op defaults. Pointer identity only
    // — never dereferenced past the cast
    [[nodiscard]] AbstractFileModel* activeFileModel() const
    {
        auto* view = activeFileView();
        return view ? view->model() : nullptr;
    }

    void changeEvent(QEvent* event) override
    {
        QMainWindow::changeEvent(event);

        if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
            emit windowActivated();
        }
    }

    void moveEvent(QMoveEvent* event) override
    {
        QMainWindow::moveEvent(event);
        emit geometryChanged();
    }

    // The first time the window shows, focus its active tab's page. A window
    // with no focus widget of its own is given the first one in its tab order
    // when it activates, which is the sidebar's tree; and the pages a restore
    // adds lose their focus when the restored tree is put in place. Later
    // shows (un-minimizing, say) leave focus where the user left it
    void showEvent(QShowEvent* event) override
    {
        QMainWindow::showEvent(event);

        if (!event->spontaneous() && !shown_) {
            shown_ = true;
            tree_->focusActivePage();
        }
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QMainWindow::resizeEvent(event);
        emit geometryChanged();
    }

private:
    bool shown_ = false;
    // App's update checker, which the notice shows. Not owned
    Updater* updater_ = nullptr;

    TabPaneTree* tree_ = new TabPaneTree(this);
    WordCounter* wordCounter_ = new WordCounter(this);
    CursorPosition* cursorPosition_ = new CursorPosition(this);
    UpdateNotice* updateNotice_ = new UpdateNotice(updater_, this);

    // Whether either item is switched on (applyConfig)
    bool itemsEnabled_ = true;

    // The per-window action registry. Private so every entry arrives through
    // registerAction or adoptAction and nothing bypasses the window-add;
    // subclasses read it through action(). A command palette would index this.
    //
    // It is also the fan-out path for rebindable hotkeys, if they ever land.
    // Each window mints its own copy of the actions only it can perform, so a
    // rebind has to reach every open window — and the way it does that is a
    // walk of this hash, not a lookup table consulted at mint time (a window
    // minted before the rebind would keep its old sequence either way). The
    // whole mechanism is roughly:
    //
    //   for each id -> sequences in the user's overrides:
    //       if actionRegistry_ has that id: setShortcuts(sequences)
    //
    // called once per window at construction and again on every open window
    // when the config changes. Note SEQUENCES, plural: a StandardKey
    // registration expands to every platform binding (Redo is Ctrl+Y and
    // Ctrl+Shift+Z), so an override carrying a single QKeySequence would
    // silently drop the second. Defaults stay at the registerAction call site,
    // where they're readable — the override layer goes on top, which is how
    // both Qt Creator and KDE arrange it. Nothing is stubbed for this; the
    // registry is the part that needed to exist, and it does
    QHash<QString, QAction*> actionRegistry_{};

    void setup_(const AppActions& appActions)
    {
        setAttribute(Qt::WA_DeleteOnClose);

        buildSharedActions_();
        adoptAppActions_(appActions);
        setupStatusBar_();
    }

    // Items go in as permanent widgets: right-aligned, and never covered by
    // a temporary message. No size grip — the window's edges already resize
    // it, and Obsidian's bar has none
    void setupStatusBar_()
    {
        auto* bar = statusBar();
        bar->setSizeGripEnabled(false);
        bar->addPermanentWidget(updateNotice_);
        bar->addPermanentWidget(wordCounter_);
        bar->addPermanentWidget(cursorPosition_);

        connect(
            tree_,
            &TabPaneTree::activePageChanged,
            this,
            &BaseWindow::onActivePageChanged_);

        connect(
            updater_,
            &Updater::stateChanged,
            this,
            &BaseWindow::updateStatusBar_);
    }

    void updateStatusBar_()
    {
        statusBar()->setVisible(itemsEnabled_ || updater_->hasNotice());
    }

    // The items read a text editor, so only a text view gives them one; any
    // other page, or none, is nullptr and hides them. The one place the
    // window layer looks past AbstractFileView for the status bar's sake
    void onActivePageChanged_(QWidget* page)
    {
        auto* text_view = qobject_cast<TextFileView*>(page);
        auto* editor = text_view ? text_view->editor() : nullptr;

        wordCounter_->setEditor(editor);
        cursorPosition_->setEditor(editor);
    }

    // The actions both window types mint for themselves. VaultWindow appends
    // its own (menu-bar items, toggle-menu) in its own setup_, into the same
    // registry
    void buildSharedActions_()
    {
        // Undo / redo dispatch to the active view's model, which is why each
        // window mints its own rather than sharing App's. Left always-enabled:
        // undo()/redo() no-op when the stack is empty or the active page has no
        // editable model, so an always-live shortcut is harmless, and a focused
        // QLineEdit keeps its own undo because it accepts the ShortcutOverride
        // these never see. Nothing greys these when unavailable; the seam for
        // it is AbstractFileModel's undoAvailable / redoAvailable
        auto* undo = registerAction(
            ActionIds::DOCUMENT_UNDO,
            tr("Undo"),
            QKeySequence::Undo);
        connect(undo, &QAction::triggered, this, [this] {
            if (auto* model = activeFileModel()) {
                model->undo();
            }
        });

        auto* redo = registerAction(
            ActionIds::DOCUMENT_REDO,
            tr("Redo"),
            QKeySequence::Redo);
        connect(redo, &QAction::triggered, this, [this] {
            if (auto* model = activeFileModel()) {
                model->redo();
            }
        });

        // Cut, copy, paste, delete, and select all act on the text that has
        // keyboard focus (applyToFocusedText_), and on nothing when no text
        // does. Their keys are not bound here: every text widget already
        // handles them itself, and a window binding would only ever fire with
        // something else focused, taking Del from the file tree. So each shows
        // its key as a hint (hintedText_) and binds none. Always enabled, like
        // undo. Delete removes the selection only
        auto* cut = registerAction(
            ActionIds::TEXT_CUT,
            hintedText_(tr("Cut"), QKeySequence::Cut),
            QKeySequence{});
        connect(cut, &QAction::triggered, this, [] {
            applyToFocusedText_(
                [](QLineEdit* line) { line->cut(); },
                [](QPlainTextEdit* text) { text->cut(); });
        });

        auto* copy = registerAction(
            ActionIds::TEXT_COPY,
            hintedText_(tr("Copy"), QKeySequence::Copy),
            QKeySequence{});
        connect(copy, &QAction::triggered, this, [] {
            applyToFocusedText_(
                [](QLineEdit* line) { line->copy(); },
                [](QPlainTextEdit* text) { text->copy(); });
        });

        auto* paste = registerAction(
            ActionIds::TEXT_PASTE,
            hintedText_(tr("Paste"), QKeySequence::Paste),
            QKeySequence{});
        connect(paste, &QAction::triggered, this, [] {
            applyToFocusedText_(
                [](QLineEdit* line) { line->paste(); },
                [](QPlainTextEdit* text) { text->paste(); });
        });

        auto* remove = registerAction(
            ActionIds::TEXT_DELETE,
            hintedText_(tr("Delete"), QKeySequence::Delete),
            QKeySequence{});
        connect(remove, &QAction::triggered, this, [] {
            applyToFocusedText_(
                [](QLineEdit* line) {
                    if (line->hasSelectedText() && !line->isReadOnly()) {
                        line->del();
                    }
                },
                [](QPlainTextEdit* text) {
                    if (!text->isReadOnly()) {
                        text->textCursor().removeSelectedText();
                    }
                });
        });

        auto* select_all = registerAction(
            ActionIds::TEXT_SELECT_ALL,
            hintedText_(tr("Select All"), QKeySequence::SelectAll),
            QKeySequence{});
        connect(select_all, &QAction::triggered, this, [] {
            applyToFocusedText_(
                [](QLineEdit* line) { line->selectAll(); },
                [](QPlainTextEdit* text) { text->selectAll(); });
        });

        // View zoom dispatches to the active VIEW, not its model: zoom is
        // per-view presentation, unlike undo/redo which route through the
        // shared prime. Same inert shape as undo — a view that can't zoom
        // no-ops via AbstractFileView's defaults, so these stay always-enabled
        // and simply do nothing there (not greyed, as with undo/redo). In binds
        // Ctrl+= as well as Ctrl++ for the no-shift convenience
        auto* zoom_in = registerAction(
            ActionIds::VIEW_ZOOM_IN,
            tr("Zoom In"),
            QList<QKeySequence>{ QKeySequence(u"Ctrl+="_s),
                                 QKeySequence(u"Ctrl++"_s) });
        connect(zoom_in, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->zoomIn();
            }
        });

        auto* zoom_out = registerAction(
            ActionIds::VIEW_ZOOM_OUT,
            tr("Zoom Out"),
            QKeySequence(u"Ctrl+-"_s));
        connect(zoom_out, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->zoomOut();
            }
        });

        auto* zoom_reset = registerAction(
            ActionIds::VIEW_ZOOM_RESET,
            tr("Reset Zoom"),
            QKeySequence(u"Ctrl+0"_s));
        connect(zoom_reset, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->zoomReset();
            }
        });

        // Find dispatches to the active view as zoom does, and is inert the
        // same way on a view with nothing to search. The standard keys: Find
        // Next is F3 on Windows and Ctrl+G elsewhere
        auto* find = registerAction(
            ActionIds::VIEW_FIND,
            tr("Find"),
            QKeySequence::Find);
        connect(find, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->showFind();
            }
        });

        auto* replace = registerAction(
            ActionIds::VIEW_REPLACE,
            tr("Replace"),
            QKeySequence::Replace);
        connect(replace, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->showReplace();
            }
        });

        auto* find_next = registerAction(
            ActionIds::VIEW_FIND_NEXT,
            tr("Find Next"),
            QKeySequence::FindNext);
        connect(find_next, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->findNext();
            }
        });

        auto* find_previous = registerAction(
            ActionIds::VIEW_FIND_PREVIOUS,
            tr("Find Previous"),
            QKeySequence::FindPrevious);
        connect(find_previous, &QAction::triggered, this, [this] {
            if (auto* view = activeFileView()) {
                view->findPrevious();
            }
        });
    }

    // Call onLineEdit or onTextEdit on the widget with keyboard focus, as
    // whichever it is, or neither when it is something else. A menu doesn't
    // take the focus from the widget that had it, so from the menu bar this
    // is the field or editor the user was in. QPlainTextEdit covers the text
    // editor
    template <typename OnLineEditT, typename OnTextEditT>
    static void
    applyToFocusedText_(OnLineEditT onLineEdit, OnTextEditT onTextEdit)
    {
        auto* focus = QApplication::focusWidget();

        if (auto* line = qobject_cast<QLineEdit*>(focus)) {
            onLineEdit(line);
        } else if (auto* text = qobject_cast<QPlainTextEdit*>(focus)) {
            onTextEdit(text);
        }
    }

    // An action's text with a key after a tab, which a menu shows right-
    // aligned as it would a bound shortcut's. Display only: it binds nothing
    [[nodiscard]] static QString
    hintedText_(const QString& text, QKeySequence::StandardKey key)
    {
        return text + QChar(QChar::Tabulation) +
               QKeySequence(key).toString(QKeySequence::NativeText);
    }

    // Take App's actions into the registry. Both window types adopt the whole
    // set — see AppActions for why a pop-out holds one it can't show. App has
    // already connected each, so there is nothing to wire here and nothing to
    // relay back up; this window is one more surface the pointers reached. A
    // new App-owned action needs no change on this side
    void adoptAppActions_(const AppActions& appActions)
    {
        for (auto it = appActions.cbegin(), end = appActions.cend(); it != end;
             ++it) {
            adoptAction(it.key(), it.value());
        }
    }

    // Create + register the shell of an action; the caller's registerAction
    // overload applies the shortcut. The one spot QActions are constructed in
    // the window layer, so they never spread across the codebase
    QAction* makeAction_(const QString& id, const QString& text)
    {
        auto* action = new QAction(text, this);
        addAction(action);
        fileAction_(id, action);

        return action;
    }

    // The single insert. QHash::insert overwrites silently, which would leave
    // one reachable action and one window-added orphan nothing can find — so a
    // collision is an error, not a replacement
    void fileAction_(const QString& id, QAction* action)
    {
        ASSERT(
            !actionRegistry_.contains(id),
            "An action is already registered under id: {}!",
            id);

        actionRegistry_.insert(id, action);
    }
};

} // namespace Fernanda::Ui
