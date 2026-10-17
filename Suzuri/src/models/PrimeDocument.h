/*
 * Suzuri — A plain-text editor for creative writing
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

#include <QChar>
#include <QList>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QString>
#include <QTextCursor>
#include <QTextDocument>

#include <Coco/Debug.h>

#include "core/CMakeMacroGuard.h"

namespace Suzuri {

// TODO: Verify, and find a more descriptive name

// One canonical QTextDocument (the "prime") plus N registered view documents
// kept in sync with it.
//
// QTextDocument owns its layout, and QPlainTextEdit installs a
// QPlainTextDocumentLayout on whatever document it is given. Two editors
// sharing one document therefore share one layout, so wrapping is computed for
// a single width. Qt offers no supported way to get independent layout from a
// shared document, so each view gets its own document and edits are routed
// between them through the prime.
//
// Ownership: views own their own documents. This class only tracks them, and
// drops them on QObject::destroyed. Nothing here deletes a view document.
class PrimeDocument : public QObject
{
    Q_OBJECT

public:
    explicit PrimeDocument(QObject* parentTextFileModel)
        : QObject(parentTextFileModel)
    {
        auto layout = new QPlainTextDocumentLayout(document_);
        document_->setDocumentLayout(layout);

        connect(
            document_,
            &QTextDocument::modificationChanged,
            this,
            &PrimeDocument::modificationChanged);

        connect(
            document_,
            &QTextDocument::undoAvailable,
            this,
            &PrimeDocument::undoAvailable);

        connect(
            document_,
            &QTextDocument::redoAvailable,
            this,
            &PrimeDocument::redoAvailable);

        connect(
            document_,
            &QTextDocument::contentsChange,
            this,
            &PrimeDocument::contentsChange);
    }

    ~PrimeDocument() override { TRACER; }

    // --- Views -------------------------------------------------------------

    // The view keeps ownership of viewDoc. Do NOT hand out document() to a
    // view; register the view's own document here instead
    void registerView(QTextDocument* viewDoc)
    {
        if (!viewDoc || viewDocuments_.contains(viewDoc)) {
            return;
        }

        viewDocuments_ << viewDoc;
        viewDoc->setUndoRedoEnabled(false);

        // Initialize content from prime
        {
            RoutingScope_ scope(routing_);
            viewDoc->setPlainText(losslessPlainText_(document_));
        }

        connect(
            viewDoc,
            &QTextDocument::contentsChange,
            this,
            [this, viewDoc](int pos, int removed, int added) {
                onViewContentsChange_(viewDoc, pos, removed, added);
            });

        connect(viewDoc, &QObject::destroyed, this, [this, viewDoc] {
            viewDocuments_.removeAll(viewDoc);
        });

        DEBUG(
            "View document registered [{}], total views: {}",
            viewDoc,
            viewDocuments_.size());
    }

    void unregisterView(QTextDocument* viewDoc)
    {
        if (!viewDoc) {
            return;
        }

        viewDoc->disconnect(this);
        viewDocuments_.removeAll(viewDoc);
    }

    [[nodiscard]] int viewCount() const { return viewDocuments_.size(); }

    // --- Content -----------------------------------------------------------

    [[nodiscard]] QString text() const { return losslessPlainText_(document_); }

    void setText(const QString& text)
    {
        RoutingScope_ scope(routing_);
        document_->setPlainText(text);

        auto prime_text = losslessPlainText_(document_);
        for (auto* view_doc : viewDocuments_) {
            view_doc->setPlainText(prime_text);
        }

        checkSync_(__FUNCTION__);
    }

    // Replace the ENTIRE document as one undoable step, routed to every view
    // doc so open editors update in place. Unlike setText()/setPlainText, this
    // keeps the undo stack: after an external change is adopted, Ctrl+Z
    // restores the prior buffer and the next autosave rewrites it. Leaves the
    // document MODIFIED — it is a real edit; the caller
    // (TextFileModel::reloadContent) clears the flag, since the new content
    // matches disk. Clearing it is also what stops the next typed text from
    // joining this step (see TextFileModel::reloadContent)
    void replaceAllUndoable(const QString& text)
    {
        replayPrimeOperation_([this, &text] {
            QTextCursor cursor(document_);
            cursor.select(QTextCursor::Document);
            cursor.insertText(text);
        });
    }

    // Routed through replayPrimeOperation_ so the delta reaches every view
    // document; editing the prime alone would leave open views out of sync.
    // Also emits cursorPositionHint
    void insertText(const QString& text)
    {
        if (text.isEmpty()) {
            return;
        }

        replayPrimeOperation_([this, &text] {
            QTextCursor cursor(document_);
            cursor.insertText(text);
        });
    }

    // --- Undo / redo -------------------------------------------------------

    [[nodiscard]] bool isUndoAvailable() const
    {
        return document_->isUndoAvailable();
    }

    [[nodiscard]] bool isRedoAvailable() const
    {
        return document_->isRedoAvailable();
    }

    void undo()
    {
        if (routing_) {
            return;
        }

        replayPrimeOperation_([this] { document_->undo(); });
    }

    void redo()
    {
        if (routing_) {
            return;
        }

        replayPrimeOperation_([this] { document_->redo(); });
    }

    // --- Modification ------------------------------------------------------

    [[nodiscard]] bool isModified() const { return document_->isModified(); }
    void setModified(bool modified) { document_->setModified(modified); }

    // --- Access ------------------------------------------------------------

    // Const so callers can read it without being able to hand it to a view or
    // mutate it behind our back
    [[nodiscard]] const QTextDocument* document() const { return document_; }

signals:
    void contentsChange(int position, int charsRemoved, int charsAdded);
    void modificationChanged(bool changed);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

    // Undo/redo on the prime reaches view documents as manual applyDelta_ calls
    // via throwaway cursors. The editor's visible cursor is a separate object
    // that Qt only auto-positions during native undo on the editor's own
    // document. Since views never see a native undo (just an incoming text
    // edit) the editor has no reason to move its cursor to the change location.
    // The focused view responds to this by repositioning its cursor where the
    // undo/redo occurred. Regarding only responding for the currently focused
    // editor, this should still work for menus, since pressing an action closes
    // the menu and returns focus to the previously-focused widget before the
    // action's triggered() signal fires
    void cursorPositionHint(int position);

private:
    // When view A types a character, this object receives the contentsChange
    // and calls applyDelta_ on the prime and on view B's document. But applying
    // a delta to view B's document causes it to fire contentsChange too.
    // Without routing_, that signal would re-enter onViewContentsChange_, which
    // would try to apply the delta to the prime and view A again. The same
    // applies during undo/redo and setText
    bool routing_ = false;

    // Saves and restores rather than clearing unconditionally, so nesting is
    // safe: setting false in the destructor would release an outer scope early
    // if a nested one ever appeared
    struct RoutingScope_
    {
        bool& routing;
        bool previous;

        RoutingScope_(bool& r)
            : routing(r)
            , previous(r)
        {
            routing = true;
        }

        ~RoutingScope_() { routing = previous; }
        RoutingScope_(const RoutingScope_&) = delete;
        RoutingScope_& operator=(const RoutingScope_&) = delete;
    };

    QTextDocument* document_ = new QTextDocument(this);
    QList<QTextDocument*> viewDocuments_{};

    // A view's document changed. Route the delta to prime and the other views.
    //
    // A change that leaves the text as it was is no edit, and goes nowhere.
    // Qt reports a change to a document's format (its margin, say) as a
    // change to all its text, removed and added again unchanged; applied to
    // the prime, that would be an undo step and a modified file. Views set
    // their format before they're registered (TextEditor::buildDocument), and
    // this keeps one set later from becoming an edit. Only a change that
    // removes as many characters as it adds can leave the text as it was, so
    // only that one is compared
    void onViewContentsChange_(
        QTextDocument* source,
        int pos,
        int removed,
        int added)
    {
        if (routing_) {
            return;
        }

        RoutingScope_ scope(routing_);
        auto added_text = extractText_(source, pos, added);

        if (removed == added &&
            extractText_(document_, pos, removed) == added_text) {
            return;
        }

        applyDelta_(document_, pos, removed, added_text);
        routeDelta_(source, pos, removed, added_text);
        checkSync_(__FUNCTION__);
    }

    // Replay a prime operation (undo/redo/insert) out to all view documents
    template <typename OperationT>
    void replayPrimeOperation_(OperationT&& operation)
    {
        RoutingScope_ scope(routing_);
        auto hint_pos = -1;

        // TODO: hint_pos reflects the LAST contentsChange of an undo/redo that
        // makes several. For one edit, or edits close together (a pair the
        // typing helpers inserted, a selection they wrapped), it's fine. If
        // one ever spans distant positions, the cursor hint may land at the
        // wrong site. A fix might be to track all delta positions and pick the
        // most useful one (e.g., earliest)

        auto conn = connect(
            document_,
            &QTextDocument::contentsChange,
            this,
            [this, &hint_pos](int pos, int removed, int added) {
                auto text = extractText_(document_, pos, added);
                routeDelta_(nullptr, pos, removed, text);
                hint_pos = pos + text.length();
            });

        operation();
        disconnect(conn);

        if (hint_pos >= 0) {
            emit cursorPositionHint(hint_pos);
        }

        checkSync_(__FUNCTION__);
    }

    // The document's text exactly as stored, with line breaks as '\n'. NOT
    // toPlainText(): that also rewrites U+00A0 (no-break space) to a regular
    // space and U+2028 to '\n', which would silently alter the user's file on
    // the next save. toRawText() substitutes nothing, so the only conversion
    // needed is the block separator (U+2029) back to '\n' — the same one
    // extractText_ makes.
    //
    // Loading is lossy in one way this can't undo: QTextCursor::insertText
    // folds '\r\n', bare '\r', and a literal U+2029 into block breaks, so a
    // file's bare '\r' or U+2029 comes back as '\n'. Line-ending style
    // (LF/CRLF) is restored by TextFileModel, not here
    static QString losslessPlainText_(const QTextDocument* doc)
    {
        auto text = doc->toRawText();
        text.replace(QChar::ParagraphSeparator, QChar('\n'));
        return text;
    }

    static QString extractText_(QTextDocument* doc, int pos, int count)
    {
        if (count <= 0) {
            return {};
        }

        auto max_pos = doc->characterCount() - 1;
        if (pos >= max_pos) {
            return {};
        }

        QTextCursor cursor(doc);
        cursor.setPosition(pos);
        cursor.setPosition(qMin(pos + count, max_pos), QTextCursor::KeepAnchor);

        // QTextCursor::selectedText() returns paragraph breaks as
        // QChar::ParagraphSeparator (U+2029). We convert to '\n' because
        // QTextCursor::insertText() treats '\n' as a paragraph break. If Qt
        // ever changed how insertText handles '\n' vs ParagraphSeparator, the
        // fallback below (losslessPlainText_().mid()) is immune at O(N) cost

        auto text = cursor.selectedText();
        text.replace(QChar::ParagraphSeparator, QChar('\n'));
        return text;

        // return losslessPlainText_(doc).mid(pos, count);
    }

    static void applyDelta_(
        QTextDocument* doc,
        int pos,
        int removed,
        const QString& addedText)
    {
        // characterCount() includes the trailing paragraph separator; the last
        // valid cursor position is one before it
        auto max_pos = doc->characterCount() - 1;

        QTextCursor cursor(doc);
        cursor.setPosition(qMin(pos, max_pos));

        if (removed > 0) {
            cursor.setPosition(
                qMin(pos + removed, max_pos),
                QTextCursor::KeepAnchor);
        }

        cursor.insertText(addedText);
    }

    void routeDelta_(
        QTextDocument* exclude,
        int pos,
        int removed,
        const QString& addedText)
    {
        for (auto* view_doc : viewDocuments_) {
            if (view_doc != exclude) {
                applyDelta_(view_doc, pos, removed, addedText);
            }
        }
    }

    // Does this view document hold the prime's text? Every build compares
    // lengths, which costs nothing and catches a lost or doubled edit. A debug
    // build also compares the text itself, which catches an edit applied at the
    // wrong position; that reads every character of both documents, too much
    // to pay per keystroke in a large file in release
    [[nodiscard]] bool isInSync_(const QTextDocument* viewDoc) const
    {
        if (viewDoc->characterCount() != document_->characterCount()) {
            return false;
        }

#if APP_DEBUG

        return losslessPlainText_(viewDoc) == losslessPlainText_(document_);

#else

        return true;

#endif // APP_DEBUG
    }

    // Run after every routed change. A view document that has drifted from the
    // prime is reset to the prime's text: the prime holds the undo stack and is
    // what gets saved, so it is the one to keep. Left alone, each later edit
    // from that view would land in the prime at a position that means
    // something else there. The reset moves that view's cursor to the start,
    // and drops whatever the view showed that the prime never had.
    //
    // Nothing known causes drift; this is here for a bug not yet found
    void checkSync_(const char* context)
    {
        for (auto* view_doc : viewDocuments_) {
            if (isInSync_(view_doc)) {
                continue;
            }

            auto prime_text = losslessPlainText_(document_);
            auto view_text = losslessPlainText_(view_doc);

            auto min_len = qMin(prime_text.length(), view_text.length());
            auto diverge = 0;
            while (diverge < min_len &&
                   prime_text[diverge] == view_text[diverge]) {
                ++diverge;
            }

            CRITICAL(
                "Document drift detected in {}! View document [{}] out of "
                "sync and reset from the prime (prime len={}, view len={}, "
                "first divergence at pos={}, prime around=\"{}\", view "
                "around=\"{}\")",
                context,
                view_doc,
                prime_text.length(),
                view_text.length(),
                diverge,
                prime_text.mid(qMax(0, diverge - 20), 60),
                view_text.mid(qMax(0, diverge - 20), 60));

            RoutingScope_ scope(routing_);
            view_doc->setPlainText(prime_text);
        }
    }
};

} // namespace Suzuri
