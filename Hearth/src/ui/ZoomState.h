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

#include <QString>

namespace Fernanda {

using namespace Qt::StringLiterals;

// Shared zoom state for views with ZoomControl overlays. Owns mode, factor,
// and step/clamp logic. Views mutate this, then apply the result to their
// underlying widget and update ZoomControl's display text
class ZoomState
{
public:
    enum Mode
    {
        Fit,
        Fixed
    };

    enum Step
    {
        In = 1,
        Out = -1
    };

    Mode mode() const noexcept { return mode_; }
    qreal factor() const noexcept { return factor_; }

    void step(Step direction)
    {
        mode_ = Fixed;
        factor_ = qBound(
            MIN_FACTOR_,
            factor_ + (STEP_ * static_cast<int>(direction)),
            MAX_FACTOR_);
    }

    void toggleMode() { mode_ = (mode_ == Fit) ? Fixed : Fit; }

    void reset()
    {
        mode_ = Fixed;
        factor_ = 1.0;
    }

    QString displayText() const
    {
        if (mode_ == Fit) return u"Fit"_s;
        return u"%1%"_s.arg(qRound(factor_ * 100.0));
    }

private:
    Mode mode_ = Fit;
    qreal factor_ = 1.0;

    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;
};

} // namespace Fernanda
