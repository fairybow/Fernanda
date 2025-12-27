/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QClipboard>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QWidget>

#include "AbstractFileModel.h"
#include "Application.h"
#include "KeyFilter.h"
#include "TextFileModel.h"
#include "TextFileView.h"

namespace Fernanda {

QWidget* TextFileView::setupWidget()
{
    editor_ = new QPlainTextEdit(this);
    keyFilter_->setTextEdit(editor_);

    if (auto text_model = qobject_cast<TextFileModel*>(model()))
        editor_->setDocument(text_model->document());
    else
        FATAL("Could not set editor document!");

    connect(editor_, &QPlainTextEdit::selectionChanged, this, [&] {
        emit selectionChanged();
    });

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [&] {
        emit clipboardDataChanged();
    });

    return editor_;
}

} // namespace Fernanda
