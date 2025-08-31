#pragma once

#include <QFont>
#include <QObject>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextOption>
#include <QWidget>
#include <QtTypes>

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

    // Propagation

    const QFont& font() const { return editor_->font(); }
    void setFont(const QFont& font) { editor_->setFont(font); }
    bool hasCenterOnScroll() const { return editor_->centerOnScroll(); }

    void setHasCenterOnScroll(bool hasCenterOnScroll)
    {
        editor_->setCenterOnScroll(hasCenterOnScroll);
    }

    bool hasOverwrite() const { return editor_->overwriteMode(); }

    void setHasOverwrite(bool hasOverwrite)
    {
        editor_->setOverwriteMode(hasOverwrite);
    }

    qreal tabStopDistance() const { return editor_->tabStopDistance(); }

    void setTabStopDistance(qreal tabStopDistance)
    {
        editor_->setTabStopDistance(tabStopDistance);
    }

    QTextOption::WrapMode wordWrapMode() const
    {
        return editor_->wordWrapMode();
    }

    void setWordWrapMode(QTextOption::WrapMode wordWrapMode)
    {
        editor_->setWordWrapMode(wordWrapMode);
    }

    // Base methods

    virtual bool supportsEditing() const override
    {
        return editor_ && to<TextFileModel*>(model());
    }

    virtual bool hasPaste() const override
    {
        if (editor_) return editor_->canPaste();
        return false;
    }

    virtual bool hasSelection() const override
    {
        if (editor_) return editor_->textCursor().hasSelection();
        return false;
    }

    virtual void cut() override
    {
        if (editor_) editor_->cut();
    }

    virtual void copy() override
    {
        if (editor_) editor_->copy();
    }

    virtual void paste() override
    {
        if (editor_) editor_->paste();
    }

    virtual void deleteSelection() override
    {
        if (editor_) editor_->textCursor().removeSelectedText();
    }

    virtual void selectAll() override
    {
        if (editor_) editor_->selectAll();
    }

protected:
    virtual QWidget* setupWidget() override;

private:
    QPlainTextEdit* editor_ = nullptr;
};

} // namespace Fernanda
