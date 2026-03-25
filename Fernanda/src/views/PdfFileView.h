/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QPdfPageNavigator>
#include <QPdfView>
#include <QWidget>

#include "core/Debug.h"
#include "models/AbstractFileModel.h"
#include "models/PdfFileModel.h"
#include "ui/ZoomControl.h"
#include "views/AbstractFileView.h"

namespace Fernanda {

// TODO: Show page number on scroll!
// TODO: Ensure when we have zoom out that the background / non-PDF area is
// black
class PdfFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit PdfFileView(PdfFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~PdfFileView() override { TRACER; }

    virtual bool isUserEditable() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
        pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);

        auto pdf_model = qobject_cast<PdfFileModel*>(model());
        ASSERT(pdf_model, "PdfFileModel cast failed!");

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
