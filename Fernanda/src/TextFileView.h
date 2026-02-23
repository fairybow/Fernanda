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
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "PlainTextEdit.h"
#include "TextFileModel.h"
#include "Tr.h"

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

    /// TODO PD
    // Let our menu-defined shortcuts be the default by removing Qt's
    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == editor_ && event->type() == QEvent::ShortcutOverride) {
            auto key_event = static_cast<QKeyEvent*>(event);

            if (key_event->matches(QKeySequence::Undo)
                || key_event->matches(QKeySequence::Redo)) {
                event->ignore();
                return true;
            }
        }

        return AbstractFileView::eventFilter(watched, event);
    }

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

private slots:
    // TODO: State toggling
    void onEditorCustomContextMenuRequested_(const QPoint& pos)
    {
        // The menu bar shortcuts will presumably override these, but we want
        // the shortcuts to display here anyway

        MenuBuilder(MenuBuilder::ContextMenu, this)
            .action(Tr::nxUndo())
            .onUserTrigger(this, [&] { model()->undo(); })
            .shortcut(MenuShortcuts::UNDO)

            .action(Tr::nxRedo())
            .onUserTrigger(this, [&] { model()->redo(); })
            .shortcut(MenuShortcuts::REDO)

            .separator()

            .action(Tr::nxCut())
            .onUserTrigger(this, &TextFileView::cut)
            .shortcut(MenuShortcuts::CUT)

            .action(Tr::nxCopy())
            .onUserTrigger(this, &TextFileView::copy)
            .shortcut(MenuShortcuts::COPY)

            .action(Tr::nxPaste())
            .onUserTrigger(this, &TextFileView::paste)
            .shortcut(MenuShortcuts::PASTE)

            .action(Tr::nxDelete())
            .onUserTrigger(this, &TextFileView::deleteSelection)
            .shortcut(MenuShortcuts::DEL)

            .separator()

            .action(Tr::nxSelectAll())
            .onUserTrigger(this, &TextFileView::selectAll)
            .shortcut(MenuShortcuts::SELECT_ALL)

            .popup(editor_->mapToGlobal(pos));
    }
};

} // namespace Fernanda
