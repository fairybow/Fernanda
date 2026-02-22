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
    editor_ = new PlainTextEdit(this);
    editor_->installEventFilter(this);
    keyFilters_->setTextEdit(editor_);

    if (auto text_model = qobject_cast<TextFileModel*>(model())) {
        // Each view gets its own QTextDocument for independent layout. The
        // model coordinates content sync via delta relay
        auto view_doc = new QTextDocument(this);
        auto layout = new QPlainTextDocumentLayout(
            view_doc); // TODO: Just use custom PlainTextDocument class with
                       // layout already set?
        view_doc->setDocumentLayout(layout);

        text_model->registerViewDocument(view_doc);
        editor_->setDocument(view_doc);

        keyFilters_->setCompoundEditCallbacks(
            [text_model] { text_model->beginCompoundEdit(); },
            [text_model] { text_model->endCompoundEdit(); });
    } else {
        FATAL("Could not set editor document!");
    }

    connect(editor_, &PlainTextEdit::selectionChanged, this, [&] {
        emit selectionChanged();
    });

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [&] {
        emit clipboardDataChanged();
    });

    return editor_;
}

} // namespace Fernanda
