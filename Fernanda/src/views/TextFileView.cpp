/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include "views/TextFileView.h"

#include <QClipboard>
#include <QPlainTextDocumentLayout>
#include <QTextDocument>
#include <QWidget>

#include "core/Application.h"
#include "models/AbstractFileModel.h"
#include "models/TextFileModel.h"
#include "ui/PlainTextEdit.h"
#include "views/KeyFilters.h"

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
            [this](int position) {
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
        FATAL("TextFileModel cast failed!");
    }

    connect(editor_, &PlainTextEdit::selectionChanged, this, [this] {
        emit selectionChanged();
    });

    connect(
        editor_,
        &PlainTextEdit::customContextMenuRequested,
        this,
        &TextFileView::onEditorCustomContextMenuRequested_);

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [this] {
        emit clipboardDataChanged();
    });

    return editor_;
}

} // namespace Fernanda
