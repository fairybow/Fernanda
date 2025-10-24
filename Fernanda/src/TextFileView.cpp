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

#include "Application.h"
#include "IFileModel.h"
#include "TextFileModel.h"
#include "TextFileView.h"

namespace Fernanda {

QWidget* TextFileView::setupWidget()
{
    editor_ = new QPlainTextEdit(this);

    if (auto text_model = qobject_cast<TextFileModel*>(model()))
        editor_->setDocument(text_model->document());

    connect(editor_, &QPlainTextEdit::selectionChanged, this, [&] {
        emit selectionChanged();
    });

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [&] {
        emit clipboardDataChanged();
    });

    return editor_;
}

} // namespace Fernanda
