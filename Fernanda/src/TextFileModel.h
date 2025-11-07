/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QString>
#include <QTextBlock>
#include <QTextDocument>

#include "Coco/Path.h"
#include "Coco/TextIo.h" /// TODO: Replace with Fernanda version

#include "Debug.h"
#include "Enums.h"
#include "FileMeta.h"
#include "IFileModel.h"

namespace Fernanda {

// Text document implementation using QTextDocument (editing, undo/redo, and
// modification tracking). Provides automatic title generation for unsaved files
class TextFileModel : public IFileModel
{
    Q_OBJECT

public:
    explicit TextFileModel(const Coco::Path& path, QObject* parent = nullptr)
        : IFileModel(path, parent)
    {
        setup_();
    }

    virtual ~TextFileModel() override { TRACER; }

    QTextDocument* document() const noexcept { return document_; }
    virtual bool supportsModification() const override { return document_; }

    virtual SaveResult save() override
    {
        auto meta = this->meta();
        if (!meta) return SaveResult::NoOp;

        auto path = meta->path();
        if (path.isEmpty()) return SaveResult::NoOp;

        auto text = document_->toPlainText();
        auto success = Coco::TextIo::write(text, path);
        if (success) document_->setModified(false);
        return success ? SaveResult::Success : SaveResult::Fail;
    }

    virtual SaveResult saveAs(const Coco::Path& newPath) override
    {
        if (newPath.isEmpty()) return SaveResult::NoOp;

        auto meta = this->meta();
        if (!meta) return SaveResult::NoOp;

        auto text = document_->toPlainText();
        auto success = Coco::TextIo::write(text, newPath);

        if (success) {
            document_->setModified(false);
            meta->setPath(newPath);
        }

        return success ? SaveResult::Success : SaveResult::Fail;
    }

    virtual bool isModified() const override
    {
        return document_ && document_->isModified();
    }

    virtual bool hasUndo() const override
    {
        return document_ && document_->isUndoAvailable();
    }

    virtual bool hasRedo() const override
    {
        return document_ && document_->isRedoAvailable();
    }

    virtual void undo() override
    {
        if (document_) document_->undo();
    }

    virtual void redo() override
    {
        if (document_) document_->redo();
    }

private:
    QTextDocument* document_ = new QTextDocument(this);

    void setup_()
    {
        auto layout = new QPlainTextDocumentLayout(document_);
        document_->setDocumentLayout(layout);

        connect(
            document_,
            &QTextDocument::modificationChanged,
            this,
            [&](bool changed) { emit modificationChanged(changed); });

        connect(
            document_,
            &QTextDocument::undoAvailable,
            this,
            [&](bool available) { emit undoAvailable(available); });

        connect(
            document_,
            &QTextDocument::redoAvailable,
            this,
            [&](bool available) { emit redoAvailable(available); });

        connect(
            document_,
            &QTextDocument::contentsChange,
            this,
            &TextFileModel::onDocumentContentsChange_);
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
        for (auto block = document_->begin(); block.isValid();
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
