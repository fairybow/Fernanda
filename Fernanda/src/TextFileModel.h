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
    QTextDocument* document() const noexcept { return primeDocument_; }

    /// TODO PD
    void registerViewDocument(QTextDocument* viewDoc)
    {
        if (!viewDoc || viewDocuments_.contains(viewDoc)) return;

        viewDocuments_ << viewDoc;
        viewDoc->setUndoRedoEnabled(false);

        // Initialize content from prime
        syncing_ = true;
        viewDoc->setPlainText(primeDocument_->toPlainText());
        syncing_ = false;

        connect(
            viewDoc,
            &QTextDocument::contentsChange,
            this,
            [this, viewDoc](int pos, int removed, int added) {
                onViewDocChanged_(viewDoc, pos, removed, added);
            });

        connect(viewDoc, &QObject::destroyed, this, [this, viewDoc] {
            viewDocuments_.removeAll(viewDoc);
        });

        INFO(
            "View document registered [{}], total views: {}",
            (void*)viewDoc,
            viewDocuments_.size());
    }

    /// TODO PD
    void unregisterViewDocument(QTextDocument* viewDoc)
    {
        if (!viewDoc) return;
        viewDoc->disconnect(this);
        viewDocuments_.removeAll(viewDoc);
    }

    /// TODO PD
    // Call before a sequence of edits that should undo/redo as one step.
    // This opens an edit block on the prime doc so that deltas arriving
    // from a view are grouped into a single undo operation
    void beginCompoundEdit()
    {
        if (!primeDocument_) return;
        compoundCursor_ = QTextCursor(primeDocument_);
        compoundCursor_.beginEditBlock();
    }

    /// TODO PD
    void endCompoundEdit()
    {
        if (compoundCursor_.isNull()) return;
        compoundCursor_.endEditBlock();
        compoundCursor_ = QTextCursor{}; // release
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

        syncing_ = true;
        primeDocument_->setPlainText(QString::fromUtf8(data));

        for (auto& view_doc : viewDocuments_)
            view_doc->setPlainText(primeDocument_->toPlainText());

        syncing_ = false;
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
        if (!primeDocument_ || syncing_) return;
        replayPrimeOp_([&] { primeDocument_->undo(); });
    }

    /// TODO PD
    virtual void redo() override
    {
        if (!primeDocument_ || syncing_) return;
        replayPrimeOp_([&] { primeDocument_->redo(); });
    }

private:
    /// TODO PD
    // When View A types a character, the model receives the contentsChange and
    // calls applyDelta_ on the prime doc and View B's local doc. But applying a
    // delta to View B's doc causes View B's doc to also fire contentsChange.
    // Without syncing_, that signal would re-enter onViewDocChanged_, which
    // would try to apply the delta to the prime and View A again. The same
    // applies during undo/redo and setData
    bool syncing_ = false;
    QTextDocument* primeDocument_ = new QTextDocument(this);
    QList<QTextDocument*> viewDocuments_{};
    QTextCursor compoundCursor_{}; // Kept alive during compound edits

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
    // A view's local doc changed. Relay the delta to prime + other views
    // TODO: Move to slots?
    void
    onViewDocChanged_(QTextDocument* source, int pos, int removed, int added)
    {
        if (syncing_) return;
        syncing_ = true;

        auto added_text = extractText_(source, pos, added);

        applyDelta_(primeDocument_, pos, removed, added_text);
        broadcastDelta_(source, pos, removed, added_text);

        syncing_ = false;
    }

    /// TODO PD
    // Replay a prime doc operation (undo/redo) to all view docs
    template <typename Op> void replayPrimeOp_(Op&& op)
    {
        syncing_ = true;

        auto conn = connect(
            primeDocument_,
            &QTextDocument::contentsChange,
            this,
            [this](int pos, int removed, int added) {
                auto text = extractText_(primeDocument_, pos, added);
                broadcastDelta_(nullptr, pos, removed, text);
            });

        op();

        disconnect(conn);
        syncing_ = false;
    }

    /// TODO PD
    QString extractText_(QTextDocument* doc, int pos, int count)
    {
        if (count <= 0) return {};
        return doc->toPlainText().mid(pos, count);
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
    void broadcastDelta_(
        QTextDocument* exclude,
        int pos,
        int removed,
        const QString& addedText)
    {
        for (auto* view_doc : viewDocuments_) {
            if (view_doc != exclude)
                applyDelta_(view_doc, pos, removed, addedText);
        }
    }

private slots:
    /// WIP: Clean this
    void onDocumentContentsChange_(int from, int charsRemoved, int charsAdded)
    {
        (void)from;
        (void)charsRemoved;
        (void)charsAdded;

        auto meta = this->meta();
        if (!meta || meta->isOnDisk()) return;

        /// Could move the below (or portions) to Coco

        // Iterate through text blocks to find the first non-empty one
        for (auto block = primeDocument_->begin(); block.isValid();
             block = block.next()) {
            // Get trimmed text from the block
            if (auto block_text = block.text().trimmed();
                !block_text.isEmpty()) {
                // Limit title to first 27 (30 total if using ellipses)
                // characters
                auto title = block_text.left(27);

                /// TODO: I'd prefer just "rounding" to nearest word for the
                /// save file name for unsaved file
                if (block_text.length() > 27) title += "...";

                meta->setTitleOverride(title);
                return;
            }
        }

        meta->clearTitleOverride();
    }
};

} // namespace Fernanda
