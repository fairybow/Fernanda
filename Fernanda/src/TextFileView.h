#pragma once

#include <QObject>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "IFileModel.h"
#include "IFileView.h"
#include "TextFileModel.h"
#include "Utility.h"

namespace Fernanda {

// Text editing view using QPlainTextEdit for content display and editing
// operations (cut/copy/paste/select/undo/redo) with clipboard- and
// selection-change notification
class TextFileView : public IFileView
{
    Q_OBJECT

public:
    explicit TextFileView(TextFileModel* model, QWidget* parent = nullptr)
        : IFileView(model, parent)
    {
    }

    virtual ~TextFileView() override { COCO_TRACER; }

    virtual bool supportsEditing() const override
    {
        return plainTextEdit_ && to<TextFileModel*>(model());
    }

    virtual bool hasPaste() const override
    {
        if (plainTextEdit_) return plainTextEdit_->canPaste();
        return false;
    }

    virtual bool hasSelection() const override
    {
        if (plainTextEdit_) return plainTextEdit_->textCursor().hasSelection();
        return false;
    }

    virtual void cut() override
    {
        if (plainTextEdit_) plainTextEdit_->cut();
    }

    virtual void copy() override
    {
        if (plainTextEdit_) plainTextEdit_->copy();
    }

    virtual void paste() override
    {
        if (plainTextEdit_) plainTextEdit_->paste();
    }

    virtual void deleteSelection() override
    {
        if (plainTextEdit_) plainTextEdit_->textCursor().removeSelectedText();
    }

    virtual void selectAll() override
    {
        if (plainTextEdit_) plainTextEdit_->selectAll();
    }

protected:
    virtual QWidget* setupWidget() override;

private:
    QPlainTextEdit* plainTextEdit_ = nullptr;
};

} // namespace Fernanda
