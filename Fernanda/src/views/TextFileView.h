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

#include <optional>

#include <QAction>
#include <QChar>
#include <QEvent>
#include <QFont>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPoint>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/VaultConfig.h"
#include "core/WorkspaceKeys.h"
#include "core/spell/Misspelling.h"
#include "core/spell/SpellChecker.h"
#include "core/spell/WordList.h"
#include "models/TextFileModel.h"
#include "ui/widgets/MenuText.h"
#include "views/AbstractFileView.h"
#include "views/FindBar.h"
#include "views/TextEditor.h"
#include "views/TextSearch.h"
#include "views/ViewConstants.h"

namespace Fernanda {

// A text editor over a TextFileModel. Owns its own QTextDocument so its layout
// (line wrapping) is independent of every other view on the same file; the
// model's prime keeps the documents in sync.
//
// Search: a FindBar across the top, hidden until asked for, over the matches
// TextSearch finds in this view's document. The match the search is on is the
// editor's selection; the rest are tinted by the editor. Replacing is done in
// this view's document, so it reaches the buffer as typing does. Nothing of a
// search is saved.
//
// See docs/Architecture.md, "The prime document".
class TextFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit TextFileView(TextFileModel* fileModel)
        : AbstractFileView(fileModel)
        , model_(fileModel)
    {
        setup_();
    }

    ~TextFileView() override { TRACER; }

    [[nodiscard]] QPlainTextEdit* editor() const noexcept { return editor_; }

    // --- Workspace persistence -----------------------------------------------

    // Per-view state: the caret offset and the scroll position (both axes) into
    // the opaque blob. The view owns its schema; the window layer never names
    // these keys. The scroll pair mirrors PdfFileView; the horizontal value is
    // 0 while the editor wraps lines (its helpers are private, below)
    void writeViewState(QJsonObject& state) const override
    {
        state[WorkspaceKeys::VIEW_CURSOR] = cursorPosition_();
        state[WorkspaceKeys::VIEW_SCROLL_X] = scrollX_();
        state[WorkspaceKeys::VIEW_SCROLL_Y] = scrollY_();
    }

    void readViewState(const QJsonObject& state) override
    {
        setCursorPosition_(state.value(WorkspaceKeys::VIEW_CURSOR).toInt(0));

        // Both axes ride one optional — applied together on the first show (see
        // showEvent), since the scrollbar ranges are only valid post-layout
        if (state.contains(WorkspaceKeys::VIEW_SCROLL_Y) ||
            state.contains(WorkspaceKeys::VIEW_SCROLL_X)) {
            pendingScroll_ =
                QPoint{ state.value(WorkspaceKeys::VIEW_SCROLL_X).toInt(0),
                        state.value(WorkspaceKeys::VIEW_SCROLL_Y).toInt(0) };
        }
    }

    // The text font: family, size, and bold/italic from the hosting vault's
    // config (see AbstractFileView::applyConfig). Set on the editor widget
    // rather than the document: QPlainTextEdit copies its widget font into its
    // document's default font on every FontChange, so this one call restyles
    // the whole view. A font equal to the current one raises no FontChange, so
    // an unrelated config change costs nothing here.
    //
    // Then the editor's own settings, each of which also returns early when
    // unchanged. The tab width follows the font, so it is set after it
    void applyConfig(const VaultConfig& config) override
    {
        QFont font(config.textFontFamily(), config.textFontSize());
        font.setBold(config.textFontBold());
        font.setItalic(config.textFontItalic());
        editor_->setFont(font);

        editor_->setLineNumbers(config.lineNumbers());
        editor_->setWrapLines(config.wrapLines());
        editor_->setLeftRightMargin(config.leftRightMargin());
        editor_->setTabWidth(config.tabWidth());
        editor_->setCenterOnScroll(config.centerOnScroll());
        editor_->setLineHighlight(config.lineHighlight());
        editor_->setDoubleClickWhitespace(config.doubleClickWhitespace());
        editor_->setSelectionHandles(config.selectionHandles());

        // The typing helpers' switch turns them all off, each keeping its own
        // setting for when it's turned back on
        editor_->setAutoPairs(config.typingHelpers() && config.autoPairs());
        editor_->setBarging(config.typingHelpers() && config.barging());
    }

    void setSpellChecker(SpellChecker* borrowedSpellChecker) override
    {
        editor_->setSpellChecker(borrowedSpellChecker);
    }

    void setAcceptedWords(const WordList& words) override
    {
        editor_->setAcceptedWords(words);
    }

    // --- Search --------------------------------------------------------------

    // Open the find bar, or return to it. A selection within one line becomes
    // the term; the search then starts from the selection, so it lands on the
    // text that was selected
    void showFind() override
    {
        // selectedText gives a line break as a paragraph separator
        auto selected = editor_->textCursor().selectedText();
        auto fills_term = !selected.isEmpty() &&
                          !selected.contains(QChar::ParagraphSeparator);

        findBar_->show();

        // setTerm starts the search itself when the text differs
        if (fills_term && selected != findBar_->term()) {
            findBar_->setTerm(selected);
        } else {
            search_();
        }

        findBar_->focusTerm();
    }

    // The same, with the row to replace with
    void showReplace() override
    {
        findBar_->setReplaceShown(true);
        showFind();
    }

    // The match after the selection, wrapping to the first. With the bar
    // closed, opens it
    void findNext() override
    {
        if (findBar_->isHidden()) {
            showFind();
            return;
        }

        goToMatch_(
            TextSearch::nextIndex(
                matches_,
                editor_->textCursor().selectionEnd()));
    }

    // The match before the selection, wrapping to the last
    void findPrevious() override
    {
        if (findBar_->isHidden()) {
            showFind();
            return;
        }

        goToMatch_(
            TextSearch::previousIndex(
                matches_,
                editor_->textCursor().selectionStart()));
    }

signals:
    // From the context menu, for the host to pass to the vault
    void addToDictionaryRequested(const QString& word);
    void ignoreWordRequested(const QString& word);

protected:
    // Yield Ctrl+Z / Ctrl+Y to the window's document.undo / document.redo
    // actions instead of letting the editor consume them. This view's own
    // document has undo disabled — the model's prime holds the single shared
    // stack — so the editor would otherwise swallow these keys to no effect.
    // QPlainTextEdit accepts the ShortcutOverride for undo/redo to claim the
    // keystroke; we catch that override first, ignore() it so the key is NOT
    // treated as overridden, and return true so the editor never re-accepts it.
    // The app-level QAction shortcut then fires and dispatches to
    // activeFileModel() — this view's model when it holds focus. This makes the
    // action the single source of truth rather than running a second, competing
    // undo path here.
    //
    // Only these two need yielding. Every other editing op (typing, cut, paste,
    // delete, select-all) mutates the view document and the prime relays it —
    // or the view is read-only — so none of them are intercepted.
    //
    // Esc in the editor closes the find bar while it is open
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == editor_ && event->type() == QEvent::KeyPress &&
            !findBar_->isHidden()) {
            auto* key_event = static_cast<QKeyEvent*>(event);

            if (key_event->key() == Qt::Key_Escape) {
                closeFind_();
                return true;
            }
        }

        if (watched == editor_ && event->type() == QEvent::ShortcutOverride) {
            auto* key_event = static_cast<QKeyEvent*>(event);

            if (key_event->matches(QKeySequence::Undo) ||
                key_event->matches(QKeySequence::Redo)) {
                event->ignore();
                return true;
            }
        }

        return AbstractFileView::eventFilter(watched, event);
    }

    // Apply a stashed scroll once the editor is actually on screen and its
    // document has laid out. Deferred one tick past show so the scrollbar
    // ranges are valid; consumed once, so a later show (un-minimize, tab
    // re-select) never yanks the user's viewport back. A restored-but-unviewed
    // tab defers its own scroll to the moment it's first selected, which is
    // exactly right. Each axis is clamped to its own scrollbar's max at that
    // instant
    void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);

        if (pendingScroll_) {
            Coco::Time::onNextTick(this, [this] {
                if (!pendingScroll_) {
                    return;
                }

                auto* h = editor_->horizontalScrollBar();
                auto* v = editor_->verticalScrollBar();
                h->setValue(qMin(pendingScroll_->x(), h->maximum()));
                v->setValue(qMin(pendingScroll_->y(), v->maximum()));
                pendingScroll_.reset();
            });
        }
    }

private:
    TextFileModel* model_ = nullptr;
    TextEditor* editor_ = nullptr;
    FindBar* findBar_ = nullptr;

    // What the find bar's term matches in this view's document, in order.
    // Empty while the bar is closed. "Closed" is isHidden, the bar's own
    // state, and not isVisible, which is also false while this view's tab is
    // behind another
    QList<TextSearch::Match> matches_{};

    // A restored scroll position (both axes) awaiting the first show (see
    // readViewState / showEvent). Empty once applied — scrollX_ / scrollY_ then
    // report the live scrollbars
    std::optional<QPoint> pendingScroll_;

    // The caret's document offset
    [[nodiscard]] int cursorPosition_() const
    {
        return editor_->textCursor().position();
    }

    // The live scrollbar value on each axis, or the pending restored value
    // until the first show applies it — so a save before this tab is ever
    // viewed keeps the saved scroll instead of overwriting it with an
    // un-laid-out 0
    [[nodiscard]] int scrollX_() const
    {
        return pendingScroll_ ? pendingScroll_->x()
                              : editor_->horizontalScrollBar()->value();
    }

    [[nodiscard]] int scrollY_() const
    {
        return pendingScroll_ ? pendingScroll_->y()
                              : editor_->verticalScrollBar()->value();
    }

    // Restore the caret to a saved document offset. A document op, so it takes
    // effect immediately (pre-show is fine); clamped against the current length
    // in case the file shrank between sessions (an external edit)
    void setCursorPosition_(int position)
    {
        auto max = qMax(0, editor_->document()->characterCount() - 1);
        auto cursor = editor_->textCursor();
        cursor.setPosition(qBound(0, position, max));
        editor_->setTextCursor(cursor);
    }

    void setup_()
    {
        // The find bar above the editor, in one content widget. Focus given
        // to the view goes on to the editor
        auto* content = new QWidget(this);
        findBar_ = new FindBar(content);
        findBar_->hide();
        editor_ = new TextEditor(content);
        editor_->installEventFilter(this);
        content->setFocusProxy(editor_);

        auto* content_layout = new QVBoxLayout(content);
        content_layout->setContentsMargins(0, 0, 0, 0);
        content_layout->setSpacing(0);
        content_layout->addWidget(findBar_);
        content_layout->addWidget(editor_, 1);

        // Replace QPlainTextEdit's built-in context menu — whose Undo/Redo bind
        // to this view's disabled document, and which also offers cut/paste we
        // don't surface here — with our own Undo/Redo pair routed to the model.
        // The right-click event propagates from the viewport up to the editor,
        // so the editor's policy is what's honored
        editor_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(
            editor_,
            &QWidget::customContextMenuRequested,
            this,
            &TextFileView::onEditorContextMenuRequested_);

        // Each view gets its own document for independent layout. The model
        // routes content between it, the prime, and the other views' documents
        auto* view_doc = editor_->buildDocument();

        // Register BEFORE handing the doc to the editor: registerView seeds it
        // with the prime's current text and disables its per-document undo so
        // the model's single shared stack is authoritative
        model_->registerView(view_doc);
        editor_->setDocument(view_doc);

        // After an undo/redo this view didn't originate, its visible cursor has
        // no reason to move (it saw an incoming edit, not a native undo). The
        // model hints where the change was; only the focused view acts on it
        connect(
            model_,
            &TextFileModel::cursorPositionHint,
            this,
            [this](int position) {
                if (!editor_ || !editor_->hasFocus()) {
                    return;
                }

                auto cursor = editor_->textCursor();
                cursor.setPosition(position);
                editor_->setTextCursor(cursor);
                editor_->ensureCursorVisible();
            });

        setupFind_(view_doc);
        setWidget(content);
    }

    void setupFind_(QTextDocument* viewDoc)
    {
        connect(
            findBar_,
            &FindBar::searchChanged,
            this,
            &TextFileView::search_);

        connect(
            findBar_,
            &FindBar::nextRequested,
            this,
            &TextFileView::findNext);

        connect(
            findBar_,
            &FindBar::previousRequested,
            this,
            &TextFileView::findPrevious);

        connect(
            findBar_,
            &FindBar::replaceRequested,
            this,
            &TextFileView::replace_);

        connect(
            findBar_,
            &FindBar::replaceAllRequested,
            this,
            &TextFileView::replaceAll_);

        connect(
            findBar_,
            &FindBar::closeRequested,
            this,
            &TextFileView::closeFind_);

        // The text changed, here or in another view: what matched may have
        // moved, gone, or appeared. The selection is left where it is
        connect(viewDoc, &QTextDocument::contentsChange, this, [this] {
            if (!findBar_->isHidden()) {
                findMatches_();
            }
        });

        // The cursor moved: the count says whether the selection is a match
        connect(editor_, &QPlainTextEdit::cursorPositionChanged, this, [this] {
            if (!findBar_->isHidden()) {
                updateFindCount_();
            }
        });
    }

    // Find the bar's term again, tint what it matches, and show the count
    void findMatches_()
    {
        matches_ = TextSearch::findAll(
            editor_->document(),
            findBar_->term(),
            findBar_->options());

        editor_->setSearchMatches(matches_);
        updateFindCount_();
    }

    // A new term or option: find it, and go to the first match at or after
    // the start of the selection. Starting from the selection's start keeps
    // the search on one match while more of its letters are typed
    void search_()
    {
        findMatches_();

        goToMatch_(
            TextSearch::nextIndex(
                matches_,
                editor_->textCursor().selectionStart()));
    }

    // Select a match, which scrolls it into view. -1, when there are no
    // matches, does nothing
    void goToMatch_(int index)
    {
        if (index < 0) {
            return;
        }

        const auto& match = matches_.at(index);

        auto cursor = editor_->textCursor();
        cursor.setPosition(match.position);
        cursor.setPosition(
            match.position + match.length,
            QTextCursor::KeepAnchor);
        editor_->setTextCursor(cursor);

        updateFindCount_();
    }

    // The index of the match the selection covers exactly, or -1
    [[nodiscard]] int selectedMatchIndex_() const
    {
        auto cursor = editor_->textCursor();
        auto start = cursor.selectionStart();
        auto length = cursor.selectionEnd() - start;

        for (auto i = 0; i < matches_.size(); ++i) {
            const auto& match = matches_.at(i);

            if (match.position == start && match.length == length) {
                return i;
            }

            if (match.position > start) {
                break;
            }
        }

        return -1;
    }

    void updateFindCount_()
    {
        findBar_->setCount(
            selectedMatchIndex_(),
            static_cast<int>(matches_.size()));
    }

    // Replace the match the search is on, then go to the next. When the
    // selection is not a match, only goes to the next, so the first press
    // shows what the second will replace. A match that already reads as the
    // replacement is stepped over unchanged.
    //
    // The match is copied: the edit refreshes matches_ before replaceOne
    // returns
    void replace_()
    {
        auto index = selectedMatchIndex_();
        auto replacement = findBar_->replacement();

        if (index >= 0 && editor_->textCursor().selectedText() != replacement) {
            auto match = matches_.at(index);
            TextSearch::replaceOne(editor_->document(), match, replacement);
        }

        findNext();
    }

    // Replace every match, as one undo step, and say how many were replaced.
    // The edit has refreshed the matches and the count by the time
    // replaceAll returns, so the message is set after it
    void replaceAll_()
    {
        auto replaced = TextSearch::replaceAll(
            editor_->document(),
            findBar_->term(),
            findBar_->replacement(),
            findBar_->options());

        if (replaced > 0) {
            findBar_->setMessage(tr("%Ln replaced", nullptr, replaced));
        }
    }

    // Hide the bar and its row to replace with, drop the tint, and return to
    // the text. The selection stays on the match the search was on
    void closeFind_()
    {
        findBar_->hide();
        findBar_->setReplaceShown(false);
        matches_.clear();
        editor_->setSearchMatches(matches_);
        editor_->setFocus();
    }

    // The editor's context menu. Over a misspelled word, it opens with what
    // the dictionary suggests, then a way to add the word to the vault's
    // dictionary or ignore it for now; those two go out as requests, since a
    // view doesn't know its vault. Then, always, Undo/Redo, the clipboard
    // and Delete, and Select All.
    //
    // Undo/Redo are routed to the model's shared stack rather than the
    // editor's disabled document. The rest are the editor's own: an edit they
    // make reaches the buffer as typing does. Each shortcut is a display-only
    // hint (Ui::MenuText::withKey), so nothing here registers a binding that
    // competes with the window's actions or the editor's own keys. An item that
    // can't apply is disabled, and a disabled item can't be chosen, so reading
    // exec()'s return needs no availability re-check. Delete removes the
    // selection only.
    //
    // A QAbstractScrollArea gives the request's position in its viewport's
    // coordinates, so the menu is placed from the viewport
    void onEditorContextMenuRequested_(const QPoint& pos)
    {
        QMenu menu(editor_);

        auto word_cursor = editor_->wordAt(pos);
        auto word = word_cursor.selectedText();
        auto misspelled = editor_->isMisspelled(word);

        QStringList suggestions{};
        QList<QAction*> suggestion_actions{};
        QAction* add_to_dictionary = nullptr;
        QAction* ignore = nullptr;

        if (misspelled) {
            suggestions = suggestionsFor_(word);

            for (const auto& suggestion : suggestions) {
                suggestion_actions << menu.addAction(suggestion);
            }

            if (suggestion_actions.isEmpty()) {
                menu.addAction(tr("No suggestions"))->setEnabled(false);
            }

            menu.addSeparator();
            add_to_dictionary = menu.addAction(tr("Add to dictionary"));
            ignore = menu.addAction(tr("Ignore"));
            menu.addSeparator();
        }

        auto has_selection = editor_->textCursor().hasSelection();
        auto writable = !editor_->isReadOnly();

        auto* undo = menu.addAction(
            Ui::MenuText::withKey(tr("Undo"), QKeySequence::Undo));
        undo->setEnabled(model_->isUndoAvailable());

        auto* redo = menu.addAction(
            Ui::MenuText::withKey(tr("Redo"), QKeySequence::Redo));
        redo->setEnabled(model_->isRedoAvailable());

        menu.addSeparator();

        auto* cut =
            menu.addAction(Ui::MenuText::withKey(tr("Cut"), QKeySequence::Cut));
        cut->setEnabled(has_selection && writable);

        auto* copy = menu.addAction(
            Ui::MenuText::withKey(tr("Copy"), QKeySequence::Copy));
        copy->setEnabled(has_selection);

        auto* paste = menu.addAction(
            Ui::MenuText::withKey(tr("Paste"), QKeySequence::Paste));
        paste->setEnabled(editor_->canPaste());

        auto* remove = menu.addAction(
            Ui::MenuText::withKey(tr("Delete"), QKeySequence::Delete));
        remove->setEnabled(has_selection && writable);

        menu.addSeparator();

        auto* select_all = menu.addAction(
            Ui::MenuText::withKey(tr("Select All"), QKeySequence::SelectAll));
        select_all->setEnabled(!editor_->document()->isEmpty());

        auto* chosen = menu.exec(editor_->viewport()->mapToGlobal(pos));

        if (!chosen) {
            return;
        }

        if (chosen == undo) {
            model_->undo();
        } else if (chosen == redo) {
            model_->redo();
        } else if (chosen == cut) {
            editor_->cut();
        } else if (chosen == copy) {
            editor_->copy();
        } else if (chosen == paste) {
            editor_->paste();
        } else if (chosen == remove) {
            editor_->textCursor().removeSelectedText();
        } else if (chosen == select_all) {
            editor_->selectAll();
        } else if (chosen == add_to_dictionary) {
            emit addToDictionaryRequested(Misspelling::withoutPossessive(word));
        } else if (chosen == ignore) {
            emit ignoreWordRequested(Misspelling::withoutPossessive(word));
        } else if (auto i = suggestion_actions.indexOf(chosen); i >= 0) {
            // One edit, so one undo step. The text comes from the list, not
            // the action, which a style may have given a shortcut marker
            word_cursor.insertText(suggestions.at(i));
        }
    }

    // What the dictionary offers for a word. The dictionary gives a plain
    // apostrophe; a word typed with typographic ones gets them back, so a
    // correction matches the text around it
    [[nodiscard]] QStringList suggestionsFor_(const QString& word) const
    {
        auto suggestions = editor_->spellChecker()->suggestions(
            word,
            SPELLING_SUGGESTIONS_MAX);

        if (word.contains(QChar(0x2019))) {
            for (auto& suggestion : suggestions) {
                suggestion.replace(QChar(u'\''), QChar(0x2019));
            }
        }

        return suggestions;
    }
};

} // namespace Fernanda
