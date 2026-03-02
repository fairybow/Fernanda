/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QPdfPageNavigator>
#include <QPdfView>
#include <QWidget>

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Debug.h"
#include "PdfFileModel.h"

namespace Fernanda {

// TODO: Show page number on scroll!
// TODO: Need zoom controls!
// TODO: For these overlay widgets, look to SelectionHandleOverlay.h as an
// example
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
        pdf_view->setZoomMode(QPdfView::ZoomMode::FitToWidth);

        if (auto pdf_model = qobject_cast<PdfFileModel*>(model())) {
            auto document = pdf_model->document();
            pdf_view->setDocument(document);
            // pageCount_->setPageCount(document->pageCount());

            // connect(
            //     pdf_view->pageNavigator(),
            //     &QPdfPageNavigator::currentPageChanged,
            //     pageCount_,
            //     &PdfPageCountWidget::setCurrentPage);

            // connect(
            //     document,
            //     &QPdfDocument::pageCountChanged,
            //     pageCount_,
            //     &PdfPageCountWidget::setPageCount);

        } else {
            FATAL("PdfFileModel cast failed!");
        }

        return pdf_view;
    }

private:
    // PdfPageCountWidget* pageCount_ = new PdfPageCountWidget(this);
};

} // namespace Fernanda
