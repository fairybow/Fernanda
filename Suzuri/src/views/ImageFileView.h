/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QEvent>
#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHideEvent>
#include <QIODevice>
#include <QJsonObject>
#include <QMovie>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QScrollBar>
#include <QShowEvent>
#include <QSizeF>
#include <QTransform>
#include <QWheelEvent>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/WorkspaceKeys.h"
#include "models/ImageFileModel.h"
#include "views/AbstractFileView.h"
#include "views/ZoomControl.h"
#include "views/ZoomState.h"

namespace Suzuri {

// A read-only image view over an ImageFileModel. Shows the model's shared
// pixmap in a QGraphicsView — one per view, so each keeps its own zoom and
// scroll over the one set of pixels (the image analogue of each PdfFileView's
// QPdfView over the model's one QPdfDocument).
//
// Why QGraphicsView and not a scroll-area'd QLabel: the view scales at paint
// time through its transform, where a QLabel would need a freshly scaled pixmap
// per zoom step; and it pans for free. No QGraphicsView subclass: the viewport
// is filtered for wheel events instead, as in PdfFileView.
//
// Zoom — PdfFileView's model, on the same ZoomState and the same three
// entry points (the view.zoom* actions via AbstractFileView's virtuals,
// Ctrl+wheel, the ZoomControl pill), all funneling through applyZoom_:
//
//   Fit    — contain the whole image, centered, never upscaled past 100%. The
//            opening default, re-applied as the viewport resizes
//   Fixed  — the clamped factor (0.1–3.0, 0.1 steps); 100% is one image pixel
//            per logical screen pixel, as in a browser
//
// Two things PDF can't do, because here the view owns the math:
//
//   - Leaving Fit continues from what's on screen. The fit scale is computed
//     here (fitScale_), so a step from Fit passes it to ZoomState::step, which
//     snaps to the grid — PDF's documented Fit→Fixed jump doesn't exist.
//   - Ctrl+wheel is cursor-anchored: the point under the pointer stays put.
//     QGraphicsView applies its transformation anchor inside setTransform, so
//     wheel zoom sets AnchorUnderMouse and everything else AnchorViewCenter —
//     the pill sits over the view, and anchoring a pill click on the pill
//     would drift the image toward the corner.
//
// Why compute Fit rather than call fitInView: fitInView pads by a hard-coded
// 2 px, leaving a visible frame of background, and wouldn't hand back the
// scale that step-from-Fit needs.
//
// Scrollbars and panning follow the mode: Fit turns the bars OFF (never
// AsNeeded — a bar appearing mid-fit would shrink the viewport under the fit)
// and has nothing to drag; Fixed turns them AsNeeded and drag-pans
// (ScrollHandDrag).
//
// Refit on the VIEWPORT's resize, not the view's. Fitting immediately after
// turning the scrollbars off measures a viewport that hasn't yet grown into the
// space they vacated, so the fit comes out a scrollbar too small and leaves a
// black strip. The viewport's own Resize arrives only once its geometry is
// final — including after the Fixed→Fit policy change above — so fitting there
// always measures the real space.
//
// Black background: images are judged against a neutral dark field rather than
// the theme, and transparency reads as black. Smoothing is on the ITEM
// (Qt::SmoothTransformation): QGraphicsPixmapItem::paint sets the painter's
// SmoothPixmapTransform hint from its own transformationMode, so the view-level
// render hint alone is overridden to nearest-neighbor.
//
// Persistence — PdfFileView's shape exactly, through AbstractFileView's
// writeViewState/readViewState into the tab entry's opaque state blob: zoom
// (ZoomState's mode + factor) and the scroll pair. Zoom applies at once on
// restore; the scroll is stashed and applied one tick past the first showEvent,
// clamped to each bar's max, since a QGraphicsView's scroll range only exists
// once its viewport is laid out. Restoring the zoom first is what makes the
// saved scrollbar values map back onto the same part of the image. In Fit the
// bars are off and read 0, so a Fit tab saves and restores a harmless 0,0 — no
// special case. Scrollbar values are offsets in viewport pixels, so a different
// window size on relaunch lands close rather than exact (as with PDF).
//
// Animation. When the model reports more than one frame, this view runs its OWN
// QMovie — per view, so each tab keeps its own playback and a hidden one costs
// nothing — over a QBuffer on the model's bytes (a COW share of data(), not a
// copy). Each frame is pushed into the same pixmap item, so zoom, Fit, panning,
// and persistence don't know animation exists. Until the movie shows its first
// frame (and whenever it's paused) the item holds the model's static first
// frame. The movie pauses on hideEvent — a tab switched away from, a collapsed
// split, a minimized window — and resumes on showEvent; a GIF that loops a
// finite number of times stops on its last frame, as in a browser. A QMovie
// applies no EXIF orientation; animated formats rarely carry one, and the model
// treats anything it can't count as static anyway.
//
// A failed decode (null pixmap) shows the base's load-failure placeholder (the
// pill, parented to the QGraphicsView, hides with it); an external change
// re-reads the model on reloaded() and swaps either way, keeping the current
// zoom
class ImageFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit ImageFileView(ImageFileModel* fileModel)
        : AbstractFileView(fileModel)
        , model_(fileModel)
    {
        setup_();
    }

    ~ImageFileView() override { TRACER; }

    // --- AbstractFileView zoom contract ------------------------------

    // From the keyboard/menu: centered, since the pointer may be anywhere
    void zoomIn() override
    {
        stepZoom_(ZoomState::Direction::In, QGraphicsView::AnchorViewCenter);
    }

    void zoomOut() override
    {
        stepZoom_(ZoomState::Direction::Out, QGraphicsView::AnchorViewCenter);
    }

    void zoomReset() override
    {
        zoom_.reset();
        applyZoom_(QGraphicsView::AnchorViewCenter);
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

    // Zoom applies now; the scroll is stashed for the first show (needs
    // layout). Missing keys keep the opening defaults — Fit, top-left
    // (ZoomState::read clamps the factor). Read even when the decode failed,
    // so a failed tab keeps its saved state across a re-save
    void readViewState(const QJsonObject& state) override
    {
        zoom_.read(state);
        applyZoom_(QGraphicsView::AnchorViewCenter);

        // Both axes ride one optional, applied together (see showEvent)
        if (state.contains(WorkspaceKeys::VIEW_SCROLL_Y) ||
            state.contains(WorkspaceKeys::VIEW_SCROLL_X)) {
            pendingScroll_ =
                QPoint{ state.value(WorkspaceKeys::VIEW_SCROLL_X).toInt(0),
                        state.value(WorkspaceKeys::VIEW_SCROLL_Y).toInt(0) };
        }
    }

protected:
    // On the viewport: its Resize refits (Fit only — see class note), and
    // Ctrl+wheel zooms around the pointer instead of scrolling. A plain wheel
    // falls through to the QGraphicsView's own scrolling. Sign-based, one step
    // per notch, as in PdfFileView
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == graphicsView_->viewport()) {
            if (event->type() == QEvent::Resize &&
                zoom_.mode() == ZoomState::Mode::Fit) {
                applyScale_(fitScale_());
            } else if (event->type() == QEvent::Wheel) {
                auto* wheel = static_cast<QWheelEvent*>(event);

                if (wheel->modifiers().testFlag(Qt::ControlModifier)) {
                    if (wheel->angleDelta().y() > 0) {
                        stepZoom_(
                            ZoomState::Direction::In,
                            QGraphicsView::AnchorUnderMouse);
                    } else if (wheel->angleDelta().y() < 0) {
                        stepZoom_(
                            ZoomState::Direction::Out,
                            QGraphicsView::AnchorUnderMouse);
                    }

                    return true; // consumed — no scroll on a zoom gesture
                }
            }
        }

        return AbstractFileView::eventFilter(watched, event);
    }

    // Apply a stashed scroll once the viewport is laid out — one tick past the
    // first show, as PdfFileView and TextFileView do. Consumed once, so a later
    // show (tab re-select, un-minimize) never yanks the view back. Clamped per
    // axis to the bar's max at that instant
    void showEvent(QShowEvent* event) override
    {
        AbstractFileView::showEvent(event);
        setMoviePlaying_(true);

        if (pendingScroll_) {
            Coco::Time::onNextTick(this, [this] {
                if (!pendingScroll_) {
                    return;
                }

                auto* h = graphicsView_->horizontalScrollBar();
                auto* v = graphicsView_->verticalScrollBar();
                h->setValue(qMin(pendingScroll_->x(), h->maximum()));
                v->setValue(qMin(pendingScroll_->y(), v->maximum()));

                pendingScroll_.reset();
            });
        }
    }

    // Stop spending decode time on frames nobody can see (see class note)
    void hideEvent(QHideEvent* event) override
    {
        AbstractFileView::hideEvent(event);
        setMoviePlaying_(false);
    }

private:
    static constexpr QColor BACKGROUND_COLOR_ = QColor(0, 0, 0);

    ImageFileModel* model_ = nullptr;
    QGraphicsView* graphicsView_ = nullptr;
    QGraphicsScene* scene_ = nullptr;
    QGraphicsPixmapItem* pixmapItem_ = nullptr;
    ZoomControl* zoomControl_ = nullptr;

    // Only while the model's image is animated; rebuilt on every reload. Owns
    // its QBuffer (a child), which outlives the movie's destructor that way
    QMovie* movie_ = nullptr;

    // Fit is the opening default; a step flips to Fixed
    ZoomState zoom_{};

    // A restored scroll position (both axes) awaiting the first show (see
    // readViewState / showEvent). Empty once applied — scrollX_ / scrollY_ then
    // report the live scrollbars
    std::optional<QPoint> pendingScroll_;

    void setup_()
    {
        graphicsView_ = new QGraphicsView(this);
        scene_ = new QGraphicsScene(graphicsView_);
        pixmapItem_ = new QGraphicsPixmapItem{}; // scene_ owns it once added

        pixmapItem_->setTransformationMode(Qt::SmoothTransformation);
        scene_->addItem(pixmapItem_);

        graphicsView_->setScene(scene_);

        // QGraphicsView accepts drops by default, and its scene accepts every
        // drag-enter — which makes the view the drag's current target and
        // starves TabPaneTree of the moves/drop (no zones, and a release pops
        // the tab out). Opt out so drags fall through to the tree
        graphicsView_->setAcceptDrops(false);
        graphicsView_->viewport()->setAcceptDrops(false);

        graphicsView_->setFrameShape(QFrame::NoFrame);
        graphicsView_->setAlignment(Qt::AlignCenter);
        graphicsView_->setBackgroundBrush(BACKGROUND_COLOR_);
        graphicsView_->setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Resize refit + Ctrl+wheel (see eventFilter)
        graphicsView_->viewport()->installEventFilter(this);

        // The floating pill over the QGraphicsView (not its viewport, so it
        // doesn't scroll with the scene). Its intents drive the same paths as
        // the keyboard — centered, like them
        zoomControl_ = new ZoomControl(graphicsView_);
        connect(
            zoomControl_,
            &ZoomControl::zoomInRequested,
            this,
            &ImageFileView::zoomIn);
        connect(
            zoomControl_,
            &ZoomControl::zoomOutRequested,
            this,
            &ImageFileView::zoomOut);
        connect(
            zoomControl_,
            &ZoomControl::toggleFitRequested,
            this,
            &ImageFileView::toggleFit_);

        // Content first: the base's placeholder requires it
        setWidget(graphicsView_);

        // The model decoded during openModel, before this view existed, so read
        // it now; and again on every external change. This also seeds the
        // scrollbar policy, drag mode, transform, and readout
        connect(
            model_,
            &AbstractFileModel::reloaded,
            this,
            &ImageFileView::refreshPixmap_);
        refreshPixmap_();
    }

    // Pull the model's pixmap into the scene, or show the placeholder. The
    // scene rect is set to exactly the image — a scene's rect otherwise only
    // ever GROWS to its items' bounds, so a reload to a smaller image would
    // leave the old extent (and phantom scroll range) behind. Zoom is kept
    // across a reload; Fit simply refits the new image. Any old movie goes
    // first, so a reload that turns a GIF static (or broken) stops playing it
    void refreshPixmap_()
    {
        rebuildMovie_();
        auto pixmap = model_->pixmap();

        if (pixmap.isNull()) {
            pixmapItem_->setPixmap({});
            showLoadFailure(tr("This image couldn't be opened."));
            return;
        }

        pixmapItem_->setPixmap(pixmap);
        scene_->setSceneRect(pixmapItem_->boundingRect());
        showLoadedContent();
        applyZoom_(QGraphicsView::AnchorViewCenter);
    }

    // Drop any existing movie, then make a fresh one if the model's content
    // animates. Started only if the view is showing now (a reload while the
    // tab is up); otherwise the next showEvent starts it. The buffer is the
    // movie's child, so deleting the movie tears down both, device last
    void rebuildMovie_()
    {
        delete movie_;
        movie_ = nullptr;

        if (!model_->isAnimated()) {
            return;
        }

        auto* buffer = new QBuffer{};
        buffer->setData(model_->data());
        buffer->open(QIODevice::ReadOnly);

        movie_ = new QMovie(buffer, QByteArray{}, this);
        buffer->setParent(movie_);

        connect(movie_, &QMovie::frameChanged, this, [this] {
            pixmapItem_->setPixmap(movie_->currentPixmap());
        });

        if (isVisible()) {
            setMoviePlaying_(true);
        }
    }

    // Start (first time) or unpause; or pause. No-op without a movie, and a
    // finished finite-loop movie isn't restarted by a pause/resume — only a
    // NotRunning movie that has never shown a frame is started
    void setMoviePlaying_(bool playing)
    {
        if (!movie_) {
            return;
        }

        if (playing) {
            if (movie_->state() == QMovie::Paused) {
                movie_->setPaused(false);
            } else if (
                movie_->state() == QMovie::NotRunning &&
                movie_->currentFrameNumber() < 0) {
                movie_->start();
            }
        } else if (movie_->state() == QMovie::Running) {
            movie_->setPaused(true);
        }
    }

    // Step from what's DISPLAYED — the fit scale while in Fit (see class note).
    // A refused step (in at the max, out at the min) changes nothing
    void stepZoom_(
        ZoomState::Direction direction,
        QGraphicsView::ViewportAnchor anchor)
    {
        if (zoom_.step(direction, displayedScale_())) {
            applyZoom_(anchor);
        }
    }

    // Readout click: Fit<->Fixed. Fixed resumes at the last tracked factor
    // (1.0 — actual size — until stepped)
    void toggleFit_()
    {
        zoom_.toggleFit();
        applyZoom_(QGraphicsView::AnchorViewCenter);
    }

    // The one place mode is turned into widget state, and the one place the
    // readout is refreshed — so every entry point stays in sync. Policy first:
    // leaving Fixed for Fit, the bars' removal resizes the viewport, whose
    // Resize refits (mode is already Fit by now); the explicit scale below
    // then lands on the same value, so the order can't leave a strip
    void applyZoom_(QGraphicsView::ViewportAnchor anchor)
    {
        auto is_fit = zoom_.mode() == ZoomState::Mode::Fit;
        auto policy = is_fit ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded;

        graphicsView_->setHorizontalScrollBarPolicy(policy);
        graphicsView_->setVerticalScrollBarPolicy(policy);
        graphicsView_->setDragMode(
            is_fit ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);

        graphicsView_->setTransformationAnchor(anchor);
        applyScale_(is_fit ? fitScale_() : zoom_.factor());

        zoomControl_->setDisplayText(zoom_.displayText());
    }

    // setTransform (not resetTransform + scale) so the anchor is applied once,
    // against the final transform
    void applyScale_(qreal scale)
    {
        graphicsView_->setTransform(QTransform::fromScale(scale, scale));
    }

    // The live scrollbar value on each axis, for persistence — or the pending
    // restored value until the first show applies it, so a save before this
    // tab is ever viewed keeps the saved scroll instead of an un-laid-out 0
    [[nodiscard]] int scrollX_() const
    {
        return pendingScroll_ ? pendingScroll_->x()
                              : graphicsView_->horizontalScrollBar()->value();
    }

    [[nodiscard]] int scrollY_() const
    {
        return pendingScroll_ ? pendingScroll_->y()
                              : graphicsView_->verticalScrollBar()->value();
    }

    [[nodiscard]] qreal displayedScale_() const
    {
        return (zoom_.mode() == ZoomState::Mode::Fit) ? fitScale_()
                                                      : zoom_.factor();
    }

    // The scale at which the whole image fits the viewport, capped at 1.0.
    // 1.0 when there's nothing to measure, so an unlaid-out view sits at
    // identity rather than collapsing to zero; the viewport's first Resize
    // corrects it
    [[nodiscard]] qreal fitScale_() const
    {
        auto image = pixmapItem_->boundingRect().size();
        auto viewport = QSizeF(graphicsView_->viewport()->size());

        if (image.isEmpty() || viewport.isEmpty()) {
            return 1.0;
        }

        return qMin(
            1.0,
            qMin(
                viewport.width() / image.width(),
                viewport.height() / image.height()));
    }
};

} // namespace Suzuri
