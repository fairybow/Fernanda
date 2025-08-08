#pragma once

#include <QFont>
#include <QLabel>
#include <QString>
#include <Qt>
#include <QWidget>

#include "IFileView.h"
#include "TextFile.h"

class NoOpFileView : public IFileView
{
    Q_OBJECT

public:
    using IFileView::IFileView;

    virtual bool canSelect() const override { return false; }
    virtual bool canPaste() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        auto label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(24);
        font.setBold(true);
        label->setFont(font);
        label->setText(":')");
        return label;
    }
};
