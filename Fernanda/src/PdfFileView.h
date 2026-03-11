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
#include "core/Debug.h"
#include "PdfFileModel.h"
#include "ZoomControl.h"

namespace Fernanda {

// TODO: Show page number on scroll!
// TODO: Ensure when we have zoom out that the background / non-PDF area is
// black
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
        pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
        pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);

        if (auto pdf_model = qobject_cast<PdfFileModel*>(model())) {
            auto document = pdf_model->document();
            pdfView_->setDocument(document);
            // pageCount_->setPageCount(document->pageCount());

            // connect(
            //     pdfView_->pageNavigator(),
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

        connect(
            zoomControl_,
            &ZoomControl::zoomChanged,
            this,
            [this](ZoomControl::Mode mode, qreal factor) {
                if (mode == ZoomControl::Fit) {
                    pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
                } else {
                    pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);
                    pdfView_->setZoomFactor(factor);
                }
            });

        return pdfView_;
    }

private:
    QPdfView* pdfView_ = new QPdfView(this);
    ZoomControl* zoomControl_ = new ZoomControl(pdfView_);
};

} // namespace Fernanda
