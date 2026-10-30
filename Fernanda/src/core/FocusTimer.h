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

#include <QObject>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/FocusCycle.h"

namespace Fernanda {

// One vault window's focus timer: a FocusCycle run against the wall clock
// (Coco::Time::epoch()), with a ticker that runs only while a period does.
// Each tick lets the cycle end a period that has run out, and says something
// changed, so whatever shows the timer can redraw. The rules are all the
// cycle's; this only gives it the time and announces what happened.
//
// Owned by its VaultWindow, which sets its lengths from the vault's settings
// and resets it when the timer is switched off. A window's timer is its own:
// two vault windows run two timers. It stops with its window, and nothing of
// it is saved
class FocusTimer : public QObject
{
    Q_OBJECT

public:
    FocusTimer(const FocusCycle::Lengths& lengths, QObject* parentVaultWindow)
        : QObject(parentVaultWindow)
        , cycle_(lengths)
    {
    }

    ~FocusTimer() override { TRACER; }

    // For reading the state, the period, the count, and what is left (with
    // Coco::Time::epoch() as now). Changing it goes through the functions
    // below, so every change is announced
    [[nodiscard]] const FocusCycle& cycle() const noexcept { return cycle_; }

    void setLengths(const FocusCycle::Lengths& lengths)
    {
        cycle_.setLengths(lengths);
        emit changed();
    }

    void start()
    {
        cycle_.start(Coco::Time::epoch());
        afterChange_();
    }

    void pause()
    {
        cycle_.pause(Coco::Time::epoch());
        afterChange_();
    }

    void resume()
    {
        cycle_.resume(Coco::Time::epoch());
        afterChange_();
    }

    void skip()
    {
        cycle_.skip(Coco::Time::epoch());
        afterChange_();
    }

    void reset()
    {
        cycle_.reset();
        afterChange_();
    }

signals:
    // The state, the period, or the time left changed: once a second while a
    // period runs, and after every call above
    void changed();

    // A period ran out, and the next is queued. Emitted after changed
    void periodEnded();

private:
    static constexpr auto TICK_MS_ = 1000;

    FocusCycle cycle_;

    Coco::Time::Ticker* ticker_ =
        Coco::Time::newTicker(this, &FocusTimer::onTick_, TICK_MS_);

    void onTick_()
    {
        auto ended = cycle_.tick(Coco::Time::epoch());
        afterChange_();

        if (ended) {
            emit periodEnded();
        }
    }

    // The ticker runs while a period runs, and only then. Started fresh when a
    // period starts or resumes, so its first tick comes a second later
    void afterChange_()
    {
        auto running = cycle_.state() == FocusCycle::State::Running;

        if (running && !ticker_->isActive()) {
            ticker_->start();
        } else if (!running) {
            ticker_->stop();
        }

        emit changed();
    }
};

} // namespace Fernanda
