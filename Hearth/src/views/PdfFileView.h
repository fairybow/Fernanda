/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QPdfView>
#include <QWidget>

#include "core/Debug.h"
#include "models/PdfFileModel.h"
#include "ui/ZoomControl.h"
#include "ui/ZoomState.h"
#include "views/AbstractFileView.h"

namespace Hearth {

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

        auto pdf_model = qobject_cast<PdfFileModel*>(model());
        ASSERT(pdf_model, "PdfFileModel cast failed!");

        pdfView_->setDocument(pdf_model->document());

        connect(
            zoomControl_,
            &ZoomControl::stepRequested,
            this,
            [this](ZoomState::Step s) {
                zoom_.step(s);
                applyZoom_();
            });

        connect(zoomControl_, &ZoomControl::toggleModeRequested, this, [this] {
            zoom_.toggleMode();
            applyZoom_();
        });

        connect(zoomControl_, &ZoomControl::resetRequested, this, [this] {
            zoom_.reset();
            applyZoom_();
        });

        applyZoom_();

        return pdfView_;
    }

private:
    QPdfView* pdfView_ = new QPdfView(this);
    ZoomControl* zoomControl_ = new ZoomControl(pdfView_);
    ZoomState zoom_{};

    void applyZoom_()
    {
        if (zoom_.mode() == ZoomState::Fit) {
            pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
        } else {
            pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);
            pdfView_->setZoomFactor(zoom_.factor());
        }

        zoomControl_->setDisplayText(zoom_.displayText());
    }
};

} // namespace Hearth
