/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <optional>

#include <QEvent>
#include <QFrame>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfView>
#include <QPoint>
#include <QScrollBar>
#include <QShowEvent>
#include <QWheelEvent>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/WorkspaceKeys.h"
#include "models/PdfFileModel.h"
#include "views/AbstractFileView.h"
#include "views/ZoomControl.h"
#include "views/ZoomState.h"

namespace Fernanda {

// A read-only PDF view over a PdfFileModel: a QPdfView on the model's shared
// QPdfDocument, so two views of one PDF, in one window or two, share one parse
// while each keeps its own scroll and zoom. The document's load status drives
// the base's failure placeholder, so a corrupt, locked, or password-protected
// PDF shows a message instead of a blank page.
//
// Zoom is a ZoomState, as in ImageFileView, driven by the view.zoom* actions,
// Ctrl+wheel, and the ZoomControl pill, all through applyZoom_. Fit is
// QPdfView::FitToWidth, the opening default; Fixed is QPdfView::Custom at
// 0.1–3.0. Reset returns to Fit, how the PDF opened. QPdfView doesn't update
// zoomFactor() while fitting, so the first step out of Fit continues from the
// last tracked factor (1.0 until stepped), not the fitted scale: an accepted
// jump, since computing the fit scale per page would be disproportionate. Nor
// is zoom cursor-anchored: QPdfView recenters, and has no anchor to set.
//
// Zoom and scroll persist in the tab's state. Zoom applies at once; the scroll
// needs layout, so it applies one tick past the first show, as in
// TextFileView. Restoring zoom first keeps the scroll values on the same place
class PdfFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit PdfFileView(PdfFileModel* fileModel)
        : AbstractFileView(fileModel)
        , model_(fileModel)
    {
        setup_();
    }

    ~PdfFileView() override { TRACER; }

    // --- AbstractFileView zoom contract ------------------------------

    // Each step leaves Fit and moves the clamped factor; reset returns to Fit
    // at the 1.0 baseline. All three re-apply immediately
    void zoomIn() override { stepZoom_(ZoomState::Direction::In); }
    void zoomOut() override { stepZoom_(ZoomState::Direction::Out); }

    void zoomReset() override
    {
        zoom_.reset();
        applyZoom_();
    }

    // --- AbstractFileView persistence contract ------------------------

    // Zoom mode + factor and the scroll position (both axes), into the opaque
    // state blob
    void writeViewState(QJsonObject& state) const override
    {
        zoom_.write(state);
        state[WorkspaceKeys::VIEW_SCROLL_X] = scrollX_();
        state[WorkspaceKeys::VIEW_SCROLL_Y] = scrollY_();
    }

    // Zoom applies now (properties); the scroll is stashed and applied on first
    // show (needs layout). Missing keys keep the opening defaults — Fit, top-
    // left (ZoomState::read clamps the factor)
    void readViewState(const QJsonObject& state) override
    {
        zoom_.read(state);
        applyZoom_();

        // Both axes ride one optional — either we have a saved position or we
        // don't; applied together on the first show (see showEvent)
        if (state.contains(WorkspaceKeys::VIEW_SCROLL_Y) ||
            state.contains(WorkspaceKeys::VIEW_SCROLL_X)) {
            pendingScroll_ =
                QPoint{ state.value(WorkspaceKeys::VIEW_SCROLL_X).toInt(0),
                        state.value(WorkspaceKeys::VIEW_SCROLL_Y).toInt(0) };
        }
    }

protected:
    // Ctrl+wheel zooms instead of scrolling; a plain wheel falls through to the
    // QPdfView's own scrolling. Wheel events are delivered to the scroll area's
    // viewport, so that's where the filter sits (installed in setup_).
    // Sign-based stepping — one 0.1 step per notch — matches the discrete
    // action steps; a high-res trackpad steps per delta, which is accepted
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == pdfView_->viewport() && event->type() == QEvent::Wheel) {
            auto* wheel = static_cast<QWheelEvent*>(event);

            if (wheel->modifiers().testFlag(Qt::ControlModifier)) {
                if (wheel->angleDelta().y() > 0) {
                    zoomIn();
                } else if (wheel->angleDelta().y() < 0) {
                    zoomOut();
                }

                return true; // consumed — no scroll on a zoom gesture
            }
        }

        return AbstractFileView::eventFilter(watched, event);
    }

    // Apply a stashed scroll once the view is on screen and laid out — the
    // scrollbars have no valid range before then. Deferred one tick past show
    // for the same reason TextFileView defers its scroll; consumed once, so a
    // later show (un-minimize, tab re-select) never yanks the reader back. Each
    // axis is clamped to its scrollbar's max at that instant, so a document
    // that laid out shorter than last session lands as close as it can
    void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);

        if (pendingScroll_) {
            Coco::Time::onNextTick(this, [this] {
                if (!pendingScroll_) {
                    return;
                }

                auto* h = pdfView_->horizontalScrollBar();
                auto* v = pdfView_->verticalScrollBar();
                h->setValue(qMin(pendingScroll_->x(), h->maximum()));
                v->setValue(qMin(pendingScroll_->y(), v->maximum()));

                pendingScroll_.reset();
            });
        }
    }

private:
    PdfFileModel* model_ = nullptr;
    QPdfView* pdfView_ = nullptr;
    ZoomControl* zoomControl_ = nullptr;

    // Fit is the opening default; a step flips to Fixed
    ZoomState zoom_{};

    // A restored scroll position (both axes) awaiting the first show (see
    // readViewState / showEvent). Empty once applied — scrollX_ / scrollY_ then
    // report the live scrollbars
    std::optional<QPoint> pendingScroll_;

    void setup_()
    {
        // MultiPage = continuous vertical scroll through the whole document.
        // The zoom mode is NOT set here — applyZoom_ drives it off zoom_ so
        // there's one source of truth
        pdfView_ = new QPdfView(this);
        pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
        pdfView_->setFrameShape(QFrame::NoFrame); // the view is the pane
        pdfView_->setDocument(model_->document());

        // Ctrl+wheel zoom — the filter goes on the viewport, where a scroll
        // area's wheel events are delivered (see eventFilter)
        pdfView_->viewport()->installEventFilter(this);

        // The floating pill, parented to the view so it overlays the content
        // and hides with it in the failure state. Its intents drive the same
        // zoom paths as the keyboard; applyZoom_ pushes the readout text back
        zoomControl_ = new ZoomControl(pdfView_);
        connect(
            zoomControl_,
            &ZoomControl::zoomInRequested,
            this,
            &PdfFileView::zoomIn);
        connect(
            zoomControl_,
            &ZoomControl::zoomOutRequested,
            this,
            &PdfFileView::zoomOut);
        connect(
            zoomControl_,
            &ZoomControl::toggleFitRequested,
            this,
            &PdfFileView::toggleFit_);

        applyZoom_(); // seed QPdfView's mode and the readout ("Fit")

        // Before the status hookup below: the base's failure placeholder
        // requires the content widget to be set first
        setWidget(pdfView_);

        // Reflect the document's load status now and on every change. The doc
        // finished loading during openModel (before this view existed), so the
        // initial call shows the right page immediately; a later reload re-runs
        // load() on this same document and fires statusChanged again, so an
        // errored file that becomes valid — or vice versa — swaps live
        connect(
            model_->document(),
            &QPdfDocument::statusChanged,
            this,
            &PdfFileView::onDocumentStatusChanged_);
        onDocumentStatusChanged_(model_->document()->status());
    }

    // A step lands in Fixed at the clamped factor; applyZoom_ then puts
    // QPdfView in Custom at it. Always from our tracked factor, even in Fit —
    // QPdfView won't report its fit scale (the class note's accepted jump). A
    // refused step (in at the max, out at the min) changes nothing
    void stepZoom_(ZoomState::Direction direction)
    {
        if (zoom_.step(direction, zoom_.factor())) {
            applyZoom_();
        }
    }

    // Readout click: Fit<->Fixed. Fixed resumes at the last tracked factor
    // (ours is authoritative, so a toggle round-trip preserves it — the
    // QPdfView fit-factor gap doesn't bite here)
    void toggleFit_()
    {
        zoom_.toggleFit();
        applyZoom_();
    }

    // The one place QPdfView's zoom is touched, and the one place the readout
    // is refreshed — so every entry point (actions, wheel, pill) stays in sync.
    // Fit → FitToWidth (the factor is ignored — QPdfView computes it and won't
    // report it back, hence the documented step-out jump); Fixed → Custom at
    // our factor
    void applyZoom_()
    {
        if (zoom_.mode() == ZoomState::Mode::Fit) {
            pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
        } else {
            pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);
            pdfView_->setZoomFactor(zoom_.factor());
        }

        if (zoomControl_) {
            zoomControl_->setDisplayText(zoom_.displayText());
        }
    }

    // The live scrollbar value on each axis, for persistence — or the pending
    // restored value until the first show applies it, so a save before this tab
    // is ever viewed keeps the saved scroll instead of overwriting it with an
    // un-laid-out 0 (the same reason TextFileView's scroll helpers report their
    // pending value too)
    [[nodiscard]] int scrollX_() const
    {
        return pendingScroll_ ? pendingScroll_->x()
                              : pdfView_->horizontalScrollBar()->value();
    }

    [[nodiscard]] int scrollY_() const
    {
        return pendingScroll_ ? pendingScroll_->y()
                              : pdfView_->verticalScrollBar()->value();
    }

    // Only Error shows the placeholder; Loading/Ready/everything else shows the
    // QPdfView (blank while loading, rendered once ready)
    void onDocumentStatusChanged_(QPdfDocument::Status status)
    {
        if (status == QPdfDocument::Status::Error) {
            showLoadFailure(tr("This PDF couldn't be opened."));
        } else {
            showLoadedContent();
        }
    }
};

} // namespace Fernanda
