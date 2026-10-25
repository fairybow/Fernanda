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

#include <QtMinMax>
#include <QtTypes>

#include <Coco/Time.h>

namespace Fernanda {

// The focus timer's rules: focus, then a break, with a long break in place of
// the short one after every so many focus periods. A plain value, with no
// timer and no clock of its own. Every function that depends on the time takes
// it as nowMs (Coco::Time::epoch()), so a test can hand it any time it likes.
// Something that redraws while it runs keeps its own ticker and calls tick.
//
// A period runs only when started. When one runs out, tick ends it: a focus
// period that ran to its end is counted, the next period is queued, and the
// cycle waits, Ended, until it's started again. Nothing starts on its own.
//
// A long break follows the focus period that completes a set. A skipped focus
// period isn't counted and completes nothing, so a short break follows it.
// After any break comes focus.
//
// New lengths apply from the next period started. A period already running or
// paused keeps the length it started with
class FocusCycle
{
public:
    enum class Period
    {
        Focus,
        ShortBreak,
        LongBreak,
    };

    enum class State
    {
        Idle, // period() is queued and hasn't started
        Running,
        Paused,
        Ended, // the last period ran out; period() is the next, queued
    };

    struct Lengths
    {
        qint64 focusMs = 0;
        qint64 shortBreakMs = 0;
        qint64 longBreakMs = 0;

        // Focus periods per set: a long break follows each set
        int longBreakEvery = 1;
    };

    explicit FocusCycle(const Lengths& lengths)
        : lengths_(clamped_(lengths))
    {
    }

    [[nodiscard]] const Lengths& lengths() const noexcept { return lengths_; }

    void setLengths(const Lengths& lengths) { lengths_ = clamped_(lengths); }

    [[nodiscard]] State state() const noexcept { return state_; }

    // The period running or paused, or the one queued to start next
    [[nodiscard]] Period period() const noexcept { return period_; }

    // Focus periods run to their end since the cycle was made or reset
    [[nodiscard]] int completedFocusCount() const noexcept
    {
        return completedFocusCount_;
    }

    // What the current period started with, or what the queued one will
    [[nodiscard]] qint64 periodLengthMs() const
    {
        if (isUnderway_()) {
            return countdown_.durationMs();
        }

        return lengthOf_(period_);
    }

    // What is left of the period running or paused, or all of the queued one.
    // Zero once a running period has run out, until tick ends it
    [[nodiscard]] qint64 remainingMs(qint64 nowMs) const
    {
        if (isUnderway_()) {
            return countdown_.remainingMs(nowMs);
        }

        return lengthOf_(period_);
    }

    // Start the queued period. Nothing happens while one is running or paused
    void start(qint64 nowMs)
    {
        if (isUnderway_()) {
            return;
        }

        countdown_.start(lengthOf_(period_), nowMs);
        state_ = State::Running;
    }

    // Nothing happens unless a period is running
    void pause(qint64 nowMs)
    {
        if (state_ != State::Running) {
            return;
        }

        countdown_.pause(nowMs);

        // A period that has already run out isn't paused; tick ends it
        if (countdown_.isPaused()) {
            state_ = State::Paused;
        }
    }

    // Nothing happens unless a period is paused
    void resume(qint64 nowMs)
    {
        if (state_ != State::Paused) {
            return;
        }

        countdown_.resume(nowMs);
        state_ = State::Running;
    }

    // Move on to the next period without counting this one. A running period
    // is followed by the next one running; otherwise the next is queued
    void skip(qint64 nowMs)
    {
        auto was_running = state_ == State::Running;

        period_ = nextAfter_(period_, false);
        countdown_.reset();
        state_ = State::Idle;

        if (was_running) {
            start(nowMs);
        }
    }

    // Back to the start: focus queued, nothing counted
    void reset()
    {
        period_ = Period::Focus;
        completedFocusCount_ = 0;
        countdown_.reset();
        state_ = State::Idle;
    }

    // End the running period if it has run out, and say so. True once per
    // period, at the first tick at or after its end, however late that is
    bool tick(qint64 nowMs)
    {
        if (state_ != State::Running || !countdown_.isFinished(nowMs)) {
            return false;
        }

        if (period_ == Period::Focus) {
            ++completedFocusCount_;
        }

        period_ = nextAfter_(period_, true);
        countdown_.reset();
        state_ = State::Ended;
        return true;
    }

private:
    Lengths lengths_;
    Period period_ = Period::Focus;
    State state_ = State::Idle;
    int completedFocusCount_ = 0;
    Coco::Time::Countdown countdown_{};

    static Lengths clamped_(const Lengths& lengths)
    {
        auto result = lengths;
        result.focusMs = qMax(result.focusMs, qint64{ 0 });
        result.shortBreakMs = qMax(result.shortBreakMs, qint64{ 0 });
        result.longBreakMs = qMax(result.longBreakMs, qint64{ 0 });
        result.longBreakEvery = qMax(result.longBreakEvery, 1);
        return result;
    }

    [[nodiscard]] bool isUnderway_() const noexcept
    {
        return state_ == State::Running || state_ == State::Paused;
    }

    [[nodiscard]] qint64 lengthOf_(Period period) const noexcept
    {
        switch (period) {
        case Period::ShortBreak:
            return lengths_.shortBreakMs;
        case Period::LongBreak:
            return lengths_.longBreakMs;
        case Period::Focus:
        default:
            return lengths_.focusMs;
        }
    }

    // completed says whether the period ran to its end. Only a focus period
    // that did, and that completes a set, is followed by a long break; the
    // count already includes it
    [[nodiscard]] Period nextAfter_(Period period, bool completed) const
    {
        if (period != Period::Focus) {
            return Period::Focus;
        }

        auto completes_set =
            completed && completedFocusCount_ % lengths_.longBreakEvery == 0;

        return completes_set ? Period::LongBreak : Period::ShortBreak;
    }
};

} // namespace Fernanda
