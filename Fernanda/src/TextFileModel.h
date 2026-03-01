/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include "Coco/Path.h"

#include "AbstractFileModel.h"
#include "Debug.h"
#include "FileMeta.h"
#include "Version.h"

namespace Fernanda {

// Text document implementation using QTextDocument (editing, undo/redo, and
// modification tracking). Provides automatic title generation for unsaved files
class TextFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    explicit TextFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : AbstractFileModel(path, parent)
    {
        setup_();
    }

    virtual ~TextFileModel() override { TRACER; }

    // The prime document. Views should NOT call setDocument() with this; use
    // registerViewDocument() instead
    // TODO: Unused
    // QTextDocument* document() const noexcept { return primeDocument_; }

    /// TODO PD
    void registerViewDocument(QTextDocument* viewDoc)
    {
        if (!viewDoc || localViewDocuments_.contains(viewDoc)) return;

        localViewDocuments_ << viewDoc;
        viewDoc->setUndoRedoEnabled(false);

        // Initialize content from prime
        {
            DeltaRoutingScope_ scope(routingDelta_);
            viewDoc->setPlainText(primeDocument_->toPlainText());
        }

        connect(
            viewDoc,
            &QTextDocument::contentsChange,
            this,
            [&, viewDoc](int pos, int removed, int added) {
                onLocalViewContentsChange_(viewDoc, pos, removed, added);
            });

        connect(viewDoc, &QObject::destroyed, this, [&, viewDoc] {
            localViewDocuments_.removeAll(viewDoc);
        });

        INFO(
            "Local view document registered [{}], total views: {}",
            viewDoc,
            localViewDocuments_.size());
    }

    /// TODO PD
    void unregisterViewDocument(QTextDocument* viewDoc)
    {
        if (!viewDoc) return;
        viewDoc->disconnect(this);
        localViewDocuments_.removeAll(viewDoc);
    }

    /// TODO PD
    // Call before a sequence of edits that should undo/redo as one step.
    // This opens an edit block on the prime doc so that deltas arriving
    // from a view are grouped into a single undo operation
    void beginCompoundEdit()
    {
        if (!primeDocument_) return;
        primeDocumentEditBlockCursor_ = QTextCursor(primeDocument_);
        primeDocumentEditBlockCursor_.beginEditBlock();
    }

    /// TODO PD
    void endCompoundEdit()
    {
        if (primeDocumentEditBlockCursor_.isNull()) return;
        primeDocumentEditBlockCursor_.endEditBlock();
        primeDocumentEditBlockCursor_ = QTextCursor{}; // release
    }

    virtual QString preferredExtension() const override
    {
        return QStringLiteral(".txt");
    }

    virtual QByteArray data() const override
    {
        return primeDocument_->toPlainText().toUtf8();
    }

    /// TODO PD
    virtual void setData(const QByteArray& data) override
    {
        if (!primeDocument_) return;

        DeltaRoutingScope_ scope(routingDelta_);
        primeDocument_->setPlainText(QString::fromUtf8(data));

        auto prime_text = primeDocument_->toPlainText();

        for (auto& view_doc : localViewDocuments_)
            view_doc->setPlainText(prime_text);

        assertSync_(__FUNCTION__);
    }

    virtual bool supportsModification() const override
    {
        return primeDocument_;
    }

    virtual bool isModified() const override
    {
        return primeDocument_ && primeDocument_->isModified();
    }

    virtual void setModified(bool modified) override
    {
        if (primeDocument_) primeDocument_->setModified(modified);
    }

    virtual bool hasUndo() const override
    {
        return primeDocument_ && primeDocument_->isUndoAvailable();
    }

    virtual bool hasRedo() const override
    {
        return primeDocument_ && primeDocument_->isRedoAvailable();
    }

    /// TODO PD
    virtual void undo() override
    {
        if (!primeDocument_ || routingDelta_) return;
        replayPrimeOperation_([&] { primeDocument_->undo(); });
    }

    /// TODO PD
    virtual void redo() override
    {
        if (!primeDocument_ || routingDelta_) return;
        replayPrimeOperation_([&] { primeDocument_->redo(); });
    }

signals:
    // Undo/redo on the prime document reaches view documents as manual
    // applyDelta_ calls via throwaway cursors. The editor's visible cursor is a
    // separate object that Qt only auto-positions during native undo on the
    // editor's own document. Since views never see a native undo (just an
    // incoming text edit) the editor has no reason to move its cursor to the
    // change location. The focused view responds to this by repositioning its
    // cursor where the undo/redo occurred. Regarding only responding for
    // currently focused editor, this should still work for menus, since when
    // pressing an action, the menu closes and returns focus to the
    // previously-focused widget before the action's triggered() signal fires
    void cursorPositionHint(int position);

private:
    /// TODO PD
    // When View A types a character, the model receives the contentsChange and
    // calls applyDelta_ on the prime doc and View B's local doc. But applying a
    // delta to View B's doc causes View B's doc to also fire contentsChange.
    // Without routingDelta_, that signal would re-enter
    // onLocalViewContentsChange_, which would try to apply the delta to the
    // prime and View A again. The same applies during undo/redo and setData
    bool routingDelta_ = false;

    /// TODO PD
    struct DeltaRoutingScope_
    {
        bool& routing;

        DeltaRoutingScope_(bool& r)
            : routing(r)
        {
            routing = true;
        }

        ~DeltaRoutingScope_() { routing = false; }
        DeltaRoutingScope_(const DeltaRoutingScope_&) = delete;
        DeltaRoutingScope_& operator=(const DeltaRoutingScope_&) = delete;
    };

    QTextDocument* primeDocument_ = new QTextDocument(this);
    QList<QTextDocument*> localViewDocuments_{};
    QTextCursor primeDocumentEditBlockCursor_{};

    void setup_()
    {
        auto layout = new QPlainTextDocumentLayout(primeDocument_);
        primeDocument_->setDocumentLayout(layout);

        connect(
            primeDocument_,
            &QTextDocument::modificationChanged,
            this,
            [&](bool changed) { emit modificationChanged(changed); });

        connect(
            primeDocument_,
            &QTextDocument::undoAvailable,
            this,
            [&](bool available) { emit undoAvailable(available); });

        connect(
            primeDocument_,
            &QTextDocument::redoAvailable,
            this,
            [&](bool available) { emit redoAvailable(available); });

        connect(
            primeDocument_,
            &QTextDocument::contentsChange,
            this,
            &TextFileModel::onDocumentContentsChange_);
    }

    /// TODO PD
    // A view's local doc changed. Route the delta to prime and other views
    void onLocalViewContentsChange_(
        QTextDocument* source,
        int pos,
        int removed,
        int added)
    {
        if (routingDelta_) return;

        DeltaRoutingScope_ scope(routingDelta_);
        auto added_text = extractText_(source, pos, added);

        applyDelta_(primeDocument_, pos, removed, added_text);
        routeDelta_(source, pos, removed, added_text);
        assertSync_(__FUNCTION__);
    }

    /// TODO PD
    // Replay a prime doc operation (undo/redo) to all view docs
    template <typename OperationT>
    void replayPrimeOperation_(OperationT&& operation)
    {
        DeltaRoutingScope_ scope(routingDelta_);
        auto hint_pos = -1;

        // TODO: hint_pos reflects the LAST contentsChange during a compound
        // undo/redo. For adjacent edits (auto-close, barge, delete-pair) it's
        // fine. If a future compound edit spans distant positions, the cursor
        // hint may land at the wrong site. A fix might be to track all delta
        // positions and pick the most useful one (e.g., earliest)

        auto conn = connect(
            primeDocument_,
            &QTextDocument::contentsChange,
            this,
            [this, &hint_pos](int pos, int removed, int added) {
                auto text = extractText_(primeDocument_, pos, added);
                routeDelta_(nullptr, pos, removed, text);
                hint_pos = pos + text.length();
            });

        operation();
        disconnect(conn);
        if (hint_pos >= 0) emit cursorPositionHint(hint_pos);
        assertSync_(__FUNCTION__);
    }

    /// TODO PD
    QString extractText_(QTextDocument* doc, int pos, int count)
    {
        if (count <= 0) return {};

        auto max_pos = doc->characterCount() - 1;
        if (pos >= max_pos) return {};

        QTextCursor cursor(doc);
        cursor.setPosition(pos);
        cursor.setPosition(qMin(pos + count, max_pos), QTextCursor::KeepAnchor);

        // QTextCursor::selectedText() returns paragraph breaks as
        // QChar::ParagraphSeparator (U+2029). We convert to '\n' because
        // QTextCursor::insertText() treats '\n' as a paragraph break. If Qt
        // ever changed how insertText handles '\n' vs ParagraphSeparator, the
        // fallback below (toPlainText().mid()) is immune at O(N) cost

        auto text = cursor.selectedText();
        text.replace(QChar::ParagraphSeparator, QChar('\n'));
        return text;

        // If the above doesn't work (but this is not ideal):
        // if (count <= 0) return {};
        // return doc->toPlainText().mid(pos, count);
    }

    /// TODO PD
    void applyDelta_(
        QTextDocument* doc,
        int pos,
        int removed,
        const QString& addedText)
    {
        // characterCount() includes the trailing paragraph separator;
        // the last valid cursor position is one before it
        auto max_pos = doc->characterCount() - 1;

        QTextCursor cursor(doc);
        cursor.setPosition(qMin(pos, max_pos));

        if (removed > 0)
            cursor.setPosition(
                qMin(pos + removed, max_pos),
                QTextCursor::KeepAnchor);

        cursor.insertText(addedText);
    }

    /// TODO PD
    void routeDelta_(
        QTextDocument* exclude,
        int pos,
        int removed,
        const QString& addedText)
    {
        for (auto* view_doc : localViewDocuments_) {
            if (view_doc != exclude)
                applyDelta_(view_doc, pos, removed, addedText);
        }
    }

    /// TODO PD
    void assertSync_(const char* context)
    {
#ifdef VERSION_DEBUG

        auto prime_text = primeDocument_->toPlainText();
        for (auto& view_doc : localViewDocuments_) {
            auto view_text = view_doc->toPlainText();
            if (view_text != prime_text) {
                auto min_len = qMin(prime_text.length(), view_text.length());
                auto diverge = 0;
                while (diverge < min_len
                       && prime_text[diverge] == view_text[diverge])
                    ++diverge;

                // TODO: Organize this lol
                FATAL(
                    "Document drift detected in {}! Local view document [{}] "
                    "out of sync (prime len={}, view len={}, "
                    "first divergence at pos={}, "
                    "prime around=\"{}\", view around=\"{}\")",
                    context,
                    view_doc,
                    prime_text.length(),
                    view_text.length(),
                    diverge,
                    prime_text.mid(qMax(0, diverge - 20), 60),
                    view_text.mid(qMax(0, diverge - 20), 60));
            }
        }

#endif
    }

private slots:
    // TODO: Clean this
    // TODO: Rename (titleChange_ or similar)?
    void onDocumentContentsChange_(int from, int charsRemoved, int charsAdded)
    {
        (void)from;
        (void)charsRemoved;
        (void)charsAdded;

        auto meta = this->meta();
        if (!meta || meta->isOnDisk()) return;

        // TODO: Could move the below (or portions) to Coco

        // Iterate through text blocks to find the first non-empty one
        for (auto block = primeDocument_->begin(); block.isValid();
             block = block.next()) {
            // Get trimmed text from the block
            if (auto block_text = block.text().trimmed();
                !block_text.isEmpty()) {
                // Limit title to first 27 (30 total if using ellipses)
                // characters
                auto title = block_text.left(27);

                // TODO: I'd prefer just "rounding" to nearest word for the save
                // file name for unsaved file
                if (block_text.length() > 27) title += QStringLiteral("...");

                meta->setTitleOverride(title);
                return;
            }
        }

        meta->clearTitleOverride();
    }
};

} // namespace Fernanda
