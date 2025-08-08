#pragma once

#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QString>
#include <QTextBlock>
#include <QTextDocument>

#include "Coco/Path.h"
#include "Coco/TextIo.h"

#include "IFile.h"

class TextFile : public IFile
{
    Q_OBJECT

public:
    explicit TextFile(const Coco::Path& path = {}, QObject* parent = nullptr)
        : IFile(path, parent)
    {
        initialize_();
    }

    QTextDocument* document() const noexcept { return document_; }

private:
    QTextDocument* document_ = new QTextDocument(this);

    void initialize_()
    {
        auto layout = new QPlainTextDocumentLayout(document_);
        document_->setDocumentLayout(layout);

        auto path = this->path();

        if (!path.isEmpty())
        {
            auto text = Coco::TextIo::read(path);
            document_->setPlainText(text);
            document_->setModified(false); // Pretty important!
        }

        connect
        (
            document_,
            &QTextDocument::contentsChange,
            this,
            &TextFile::onDocumentContentsChange_
        );

        // Propagated emissions
        connect
        (
            document_,
            &QTextDocument::modificationChanged,
            this,
            [&](bool changed) { emit modificationChanged(changed); }
        );

        connect
        (
            document_,
            &QTextDocument::undoAvailable,
            this,
            [&](bool available) { emit undoAvailable(available); }
        );

        connect
        (
            document_,
            &QTextDocument::redoAvailable,
            this,
            [&](bool available) { emit redoAvailable(available); }
        );

        // Selection
    }

private slots:
    // Todo: Fix notes here...
    void onDocumentContentsChange_(int from, int charsRemoved, int charsAdded)
    {
        // Also, make a way to open untitled in more than one view to test this...

        if (isOnDisk()) return;

        /// Could move the below (or portions) to Coco

        // Iterate through text blocks to find the first non-empty one
        for (auto block = document_->begin(); block.isValid(); block = block.next())
        {
            // Get trimmed text from the block
            if (auto block_text = block.text().trimmed(); !block_text.isEmpty())
            {
                // Limit title to first 27 (30 total if using ellipses)
                // characters
                auto title = block_text.left(27);

                /// TODO: I'd prefer just "rounding" to nearest word for the
                /// save file name for unsaved file
                if (block_text.length() > 27)
                    title += "...";

                setTemporaryTitle(title);
                return;
            }
        }

        clearTemporaryTitle();
    }

    /// ======================================================================== ///
    /// *** OVERRIDES ***                                                        ///
    /// ======================================================================== ///

public:
    virtual bool canEdit() const override { return document_; }

    virtual Save save() override
    {
        auto path = this->path();
        if (path.isEmpty()) return Save::NoOp;

        auto text = document_->toPlainText();
        auto success = Coco::TextIo::write(text, path);
        if (success) document_->setModified(false);
        return success ? Save::Success : Save::Fail;
    }

    virtual QString preferredExt() const override { return ".txt"; }
    virtual bool isEdited() const override { return document_ && document_->isModified(); }
    virtual bool hasUndo() const override { return document_ && document_->isUndoAvailable(); }
    virtual bool hasRedo() const override { return document_ && document_->isRedoAvailable(); }
    virtual void undo() override { if (document_) document_->undo(); }
    virtual void redo() override { if (document_) document_->redo(); }
};
