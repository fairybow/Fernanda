#include <QClipboard>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QWidget>

#include "Application.h"
#include "IFileModel.h"
#include "TextFileModel.h"
#include "TextFileView.h"
#include "Utility.h"

namespace Fernanda {

QWidget* TextFileView::setupWidget()
{
    editor_ = new QPlainTextEdit(this);

    if (auto text_model = to<TextFileModel*>(model()))
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
