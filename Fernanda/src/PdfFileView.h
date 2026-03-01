/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QLabel> /// Test
#include <QPdfView>
#include <QWidget>

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Debug.h"
#include "PdfFileModel.h"

namespace Fernanda {

class PdfFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit PdfFileView(PdfFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~PdfFileView() override { TRACER; }

    virtual bool supportsEditing() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        auto label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(24);
        font.setBold(true);
        label->setFont(font);
        label->setText(QStringLiteral("PDF FILE VIEW TEST"));
        return label;
    }

private:
    //
};

} // namespace Fernanda
