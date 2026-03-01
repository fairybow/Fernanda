/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "TextFileView.h"

#include <QClipboard>
#include <QPlainTextDocumentLayout>
#include <QTextDocument>
#include <QWidget>

#include "AbstractFileModel.h"
#include "Application.h"
#include "KeyFilters.h"
#include "PlainTextEdit.h"
#include "TextFileModel.h"

namespace Fernanda {

/// TODO PD
QWidget* TextFileView::setupWidget()
{
    // TODO: Move these inside the model type deduction below?
    editor_ = new PlainTextEdit(this);
    editor_->installEventFilter(this);
    editor_->setContextMenuPolicy(Qt::CustomContextMenu);

    keyFilters_->setTextEdit(editor_);

    if (auto text_model = qobject_cast<TextFileModel*>(model())) {
        // Each view gets its own QTextDocument for independent layout. The
        // model coordinates content sync via delta relay
        auto view_doc = new QTextDocument(this);
        // TODO: Just use custom PlainTextDocument class with layout already
        // set?
        auto layout = new QPlainTextDocumentLayout(view_doc);
        view_doc->setDocumentLayout(layout);

        text_model->registerViewDocument(view_doc);
        editor_->setDocument(view_doc);

        connect(
            text_model,
            &TextFileModel::cursorPositionHint,
            this,
            [&](int position) {
                if (!editor_ || !editor_->hasFocus()) return;
                auto cursor = editor_->textCursor();
                cursor.setPosition(position);
                editor_->setTextCursor(cursor);
                editor_->ensureCursorVisible();
            });

        connect(
            keyFilters_,
            &KeyFilters::multiStepEditBegan,
            text_model,
            &TextFileModel::beginCompoundEdit);

        connect(
            keyFilters_,
            &KeyFilters::multiStepEditEnded,
            text_model,
            &TextFileModel::endCompoundEdit);

    } else {
        FATAL("Could not set editor document!");
    }

    connect(editor_, &PlainTextEdit::selectionChanged, this, [&] {
        emit selectionChanged();
    });

    connect(
        editor_,
        &PlainTextEdit::customContextMenuRequested,
        this,
        &TextFileView::onEditorCustomContextMenuRequested_);

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [&] {
        emit clipboardDataChanged();
    });

    return editor_;
}

} // namespace Fernanda
