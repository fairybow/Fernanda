/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

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
        auto pdf_view = new QPdfView(this);
        pdf_view->setPageMode(QPdfView::PageMode::MultiPage);

        // TODO: Need zoom controls!
        pdf_view->setZoomMode(QPdfView::ZoomMode::FitToWidth);

        if (auto pdf_model = qobject_cast<PdfFileModel*>(model())) {
            pdf_view->setDocument(pdf_model->document());
        } else {
            FATAL("Could not set PDF document!");
        }

        return pdf_view;
    }

private:
    //
};

} // namespace Fernanda
