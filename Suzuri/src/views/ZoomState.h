/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <cmath>

#include <QCoreApplication>
#include <QJsonObject>
#include <QString>
#include <QtMinMax>

#include "core/WorkspaceKeys.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// A scaled view's zoom state: a Fit/Fixed mode and a clamped factor, plus
// everything that only depends on those two — stepping, the Fit toggle, reset,
// the readout text, and the workspace.json read/write. A plain value type, no
// QObject: the owning view holds one, calls into it from every entry point (the
// view.zoom* actions, Ctrl+wheel, the ZoomControl pill), and then applies
// mode() / factor() to its own widget. The view stays the single source of
// truth for the RENDERED zoom; this is just the arithmetic the two scaled views
// share.
//
// What Fit means is the view's business — FitToWidth for PDF, contain-and-
// never-upscale for an image. This type never computes a fit scale; a view
// that knows its own passes it into step() as the scale to step from.
//
// Constants stay private here: this is their only consumer
class ZoomState
{
public:
    enum class Mode
    {
        Fit,
        Fixed
    };

    enum class Direction
    {
        In,
        Out
    };

    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] qreal factor() const noexcept { return factor_; }

    // Step one notch from fromScale — the scale the view is DISPLAYING — and
    // land in Fixed. The result snaps to the step grid: in goes to the next
    // grid value above fromScale, out to the next below, so from an off-grid
    // fit scale of 0.437 in lands on 0.5 and out on 0.4, and from an on-grid
    // factor it's exactly ±1 step. A step that can't move the requested way —
    // in at the max, or out when fromScale already sits at or below the min
    // (an image fitted under 10%) — is refused, so "zoom out" can never
    // enlarge. Returns whether anything changed.
    //
    // The image view passes its computed fit scale while in Fit, so leaving
    // Fit continues from what's on screen. PdfFileView passes factor() even
    // in Fit, because QPdfView won't report its fit scale — the documented,
    // accepted jump
    bool step(Direction direction, qreal fromScale)
    {
        // Work in whole steps so the grid is exact; the epsilon keeps a
        // floating on-grid value (0.30000000000000004) on its own notch
        auto units = fromScale / STEP_;
        auto notch = (direction == Direction::In)
                         ? std::floor(units + EPSILON_) + 1.0
                         : std::ceil(units - EPSILON_) - 1.0;
        auto target = qBound(MIN_FACTOR_, notch * STEP_, MAX_FACTOR_);

        auto moves_the_right_way = (direction == Direction::In)
                                       ? target > fromScale + EPSILON_
                                       : target < fromScale - EPSILON_;
        if (!moves_the_right_way) {
            return false;
        }

        mode_ = Mode::Fixed;
        factor_ = target;
        return true;
    }

    // Readout click: Fit<->Fixed. Fixed resumes at the last factor — ours is
    // authoritative, so a round trip preserves it
    void toggleFit() { mode_ = (mode_ == Mode::Fit) ? Mode::Fixed : Mode::Fit; }

    // Back to how a view opens: Fit, with the factor at the 1.0 baseline
    void reset()
    {
        mode_ = Mode::Fit;
        factor_ = 1.0;
    }

    // "Fit" or "N%", for the ZoomControl readout
    [[nodiscard]] QString displayText() const
    {
        if (mode_ == Mode::Fit) {
            return QCoreApplication::translate("ZoomState", "Fit");
        }

        return u"%1%"_s.arg(qRound(factor_ * 100.0));
    }

    // --- Persistence (the view's opaque state blob) --------------------------

    void write(QJsonObject& state) const
    {
        state[WorkspaceKeys::VIEW_ZOOM_MODE] =
            (mode_ == Mode::Fit) ? WorkspaceKeys::VIEW_ZOOM_FIT
                                 : WorkspaceKeys::VIEW_ZOOM_FIXED;
        state[WorkspaceKeys::VIEW_ZOOM_FACTOR] = factor_;
    }

    // Missing keys keep the opening defaults (Fit, 1.0). The factor is
    // re-clamped in case a hand-edited file carries an out-of-range value
    void read(const QJsonObject& state)
    {
        auto mode = state.value(WorkspaceKeys::VIEW_ZOOM_MODE)
                        .toString(WorkspaceKeys::VIEW_ZOOM_FIT);
        mode_ =
            (mode == WorkspaceKeys::VIEW_ZOOM_FIXED) ? Mode::Fixed : Mode::Fit;
        factor_ = qBound(
            MIN_FACTOR_,
            state.value(WorkspaceKeys::VIEW_ZOOM_FACTOR).toDouble(1.0),
            MAX_FACTOR_);
    }

private:
    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;
    static constexpr auto EPSILON_ = 1e-6;

    Mode mode_ = Mode::Fit;
    qreal factor_ = 1.0;
};

} // namespace Fernanda
