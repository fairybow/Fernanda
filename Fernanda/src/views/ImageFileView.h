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

namespace Fernanda {

// A read-only image view over an ImageFileModel's shared pixmap, in a
// QGraphicsView of its own, so each view keeps its own zoom and scroll. A
// QGraphicsView scales at paint time and pans for free, where a QLabel would
// need a new pixmap per zoom step.
//
// Zoom works as in PdfFileView (ZoomState, applyZoom_): Fit contains the image,
// centered, never past 100%; Fixed is 0.1–3.0, 100% being one image pixel per
// logical pixel. The fit scale is computed here (fitScale_), as fitInView pads
// by 2 px and returns no scale, so unlike PDF, leaving Fit steps from what's on
// screen. Ctrl+wheel holds the point under the pointer still; other zooms
// anchor on the center, as the zoom pill sits over the view.
//
// Fit turns the scrollbars off (AsNeeded would shrink the viewport under the
// fit) and refits on the VIEWPORT's Resize, which comes only once the space the
// bars left is filled; Fixed shows them as needed and drag-pans.
//
// The background is black, so images aren't judged against the theme.
// Smoothing is set on the item, whose transformationMode overrides the view's
// render hint. Zoom and scroll persist as in PdfFileView, zoom first, so the
// scroll lands on the same part of the image.
//
// An animated image runs a QMovie per view, pushing each frame into the same
// item, so nothing else knows about animation. It pauses while hidden; a finite
// loop stops on its last frame. QMovie applies no EXIF orientation, which
// animated formats rarely carry.
//
// A failed decode shows the base's placeholder. An external change re-reads the
// model, keeping the zoom
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

} // namespace Fernanda
