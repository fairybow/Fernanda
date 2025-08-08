#pragma once

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QtTypes>
#include <QWidget>

#include "IFileView.h"
#include "plain-text-edit/PlainTextEdit.h"
#include "TextFile.h"

class TextFileView : public IFileView
{
    Q_OBJECT

public:
    using IFileView::IFileView;

    const QFont& font() const { return plainTextEdit_->font(); }
    void setFont(const QFont& font) { plainTextEdit_->setFont(font); }

    bool hasCenterOnScroll() const { return plainTextEdit_->centerOnScroll(); }
    void setHasCenterOnScroll(bool hasCenterOnScroll) { plainTextEdit_->setCenterOnScroll(hasCenterOnScroll); }

    bool hasOverwrite() const { return plainTextEdit_->overwriteMode(); }
    void setHasOverwrite(bool hasOverwrite) { plainTextEdit_->setOverwriteMode(hasOverwrite); }

    qreal tabStopDistance() const { return plainTextEdit_->tabStopDistance(); }
    void setTabStopDistance(qreal tabStopDistance) { plainTextEdit_->setTabStopDistance(tabStopDistance); }

    QTextOption::WrapMode wordWrapMode() const { return plainTextEdit_->wordWrapMode(); }
    void setWordWrapMode(QTextOption::WrapMode wordWrapMode) { plainTextEdit_->setWordWrapMode(wordWrapMode); }

private:
    PlainTextEdit* plainTextEdit_ = nullptr;

    /// ======================================================================== ///
    /// *** OVERRIDES ***                                                        ///
    /// ======================================================================== ///

public:
    virtual bool canSelect() const override { return plainTextEdit_ && model(); }

    virtual bool canPaste() const override
    {
        if (plainTextEdit_) return plainTextEdit_->canPaste();
        return false;
    }

    virtual bool hasSelection() const override
    {
        if (plainTextEdit_)
            return plainTextEdit_->textCursor().hasSelection();

        return false;
    }

    virtual void cut() override { if (plainTextEdit_) plainTextEdit_->cut(); }
    virtual void copy() override { if (plainTextEdit_) plainTextEdit_->copy(); }
    virtual void paste() override { if (plainTextEdit_) plainTextEdit_->paste(); }
    virtual void deleteSelection() override { if (plainTextEdit_) plainTextEdit_->textCursor().removeSelectedText(); }
    virtual void selectAll() override { if (plainTextEdit_) plainTextEdit_->selectAll(); }

protected:
    virtual QWidget* setupWidget() override
    {
        plainTextEdit_ = new PlainTextEdit(this);
        auto text_model = model<TextFile*>();
        plainTextEdit_->setDocument(text_model->document());

        // Propagated emissions
        connect
        (
            plainTextEdit_,
            &PlainTextEdit::selectionChanged,
            this,
            [&] { emit selectionChanged(); }
        );

        connect
        (
            QApplication::clipboard(),
            &QClipboard::dataChanged,
            this,
            [=]() { emit clipboardDataChanged(); }
        );

        return plainTextEdit_;
    }
};
