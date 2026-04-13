#pragma once

#include <QString>

namespace Fernanda {

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
        factor_ =
            qBound(MIN_FACTOR_, factor_ + (STEP_ * direction), MAX_FACTOR_);
    }

    void toggleMode() { mode_ = (mode_ == Fit) ? Fixed : Fit; }

    void reset()
    {
        mode_ = Fixed;
        factor_ = 1.0;
    }

    QString displayText() const
    {
        if (mode_ == Fit) return QStringLiteral("Fit");
        return QStringLiteral("%1%").arg(qRound(factor_ * 100.0));
    }

private:
    Mode mode_ = Fit;
    qreal factor_ = 1.0;

    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;
};

} // namespace Fernanda
