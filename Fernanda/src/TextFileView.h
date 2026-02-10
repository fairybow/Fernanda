/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QObject>
#include <QTextCursor>
#include <QTextOption>
#include <QWidget>
#include <QtTypes>

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Debug.h"
#include "KeyFilters.h"
#include "PlainTextEdit.h"
#include "TextFileModel.h"

namespace Fernanda {

// Text editing view using PlainTextEdit for content display and editing
// operations (cut/copy/paste/select/undo/redo) with clipboard- and
// selection-change notification
class TextFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit TextFileView(TextFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~TextFileView() override { TRACER; }

    PlainTextEdit* editor() const noexcept { return editor_; }
    KeyFilters* keyFilters() const noexcept { return keyFilters_; }

    // Propagation

    // TODO: Don't bother wrapping these. Just use the editor (^) my dude

    // TODO: Delete once implemented as settings panel
    // (TextFileViewAttributesPanel or something)

    /*const QFont& font() const { return editor_->font(); }
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
    }*/

    // Base methods

    virtual bool supportsEditing() const override
    {
        return editor_ && qobject_cast<TextFileModel*>(model());
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
    PlainTextEdit* editor_ = nullptr;
    KeyFilters* keyFilters_ = new KeyFilters(this);
};

} // namespace Fernanda
