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
    plainTextEdit_ = new QPlainTextEdit(this);

    if (auto text_model = to<TextFileModel*>(model()))
        plainTextEdit_->setDocument(text_model->document());

    connect(plainTextEdit_, &QPlainTextEdit::selectionChanged, this, [&] {
        emit selectionChanged();
    });

    connect(Application::clipboard(), &QClipboard::dataChanged, this, [&] {
        emit clipboardDataChanged();
    });

    return plainTextEdit_;
}

} // namespace Fernanda
