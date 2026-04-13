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
/// TODO IV: Move common zoom-related code from ImageGraphicsView,
/// ImageFileView, and PdfFileView (including Mode_ enum and things that operate
/// with it, probably)
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
            [this](int direction) {
                mode_ = Fixed;
                factor_ = qBound(
                    MIN_FACTOR_,
                    factor_ + (STEP_ * direction),
                    MAX_FACTOR_);
                applyZoom_();
            });

        connect(zoomControl_, &ZoomControl::toggleModeRequested, this, [this] {
            mode_ = (mode_ == Fit) ? Fixed : Fit;
            applyZoom_();
        });

        connect(zoomControl_, &ZoomControl::resetRequested, this, [this] {
            mode_ = Fixed;
            factor_ = 1.0;
            applyZoom_();
        });

        applyZoom_();

        return pdfView_;
    }

private:
    enum Mode_
    {
        Fit,
        Fixed
    };

    QPdfView* pdfView_ = new QPdfView(this);
    ZoomControl* zoomControl_ = new ZoomControl(pdfView_);

    Mode_ mode_ = Fit;
    qreal factor_ = 1.0;

    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;

    void applyZoom_()
    {
        if (mode_ == Fit) {
            pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
            zoomControl_->setDisplayText(QStringLiteral("Fit"));
        } else {
            pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);
            pdfView_->setZoomFactor(factor_);
            zoomControl_->setDisplayText(
                QStringLiteral("%1%").arg(qRound(factor_ * 100.0)));
        }
    }
};

} // namespace Fernanda
