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

#include <QObject>
#include <QTest>
#include <QtTypes>

#include "core/FocusCycle.h"

using namespace Fernanda;

// The focus timer's rules (core/FocusCycle.h). The cycle takes the time as an
// argument, so every case runs on made-up times in milliseconds: nothing here
// waits. Lengths are short and distinct, so a length read back names its period
class FocusCycleTest : public QObject
{
    Q_OBJECT

private slots:
    void startsIdleWithFocusQueued()
    {
        auto cycle = makeCycle_(4);

        QCOMPARE(cycle.state(), FocusCycle::State::Idle);
        QCOMPARE(cycle.period(), FocusCycle::Period::Focus);
        QCOMPARE(cycle.completedFocusCount(), 0);
        QCOMPARE(cycle.remainingMs(0), FOCUS_MS_);
        QCOMPARE(cycle.periodLengthMs(), FOCUS_MS_);
    }

    void startRunsTheQueuedPeriod()
    {
        auto cycle = makeCycle_(4);
        cycle.start(100);

        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QCOMPARE(cycle.remainingMs(100), FOCUS_MS_);
        QCOMPARE(cycle.remainingMs(400), FOCUS_MS_ - 300);
    }

    void tickBeforeTheEndChangesNothing()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);

        QVERIFY(!cycle.tick(FOCUS_MS_ - 1));
        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QCOMPARE(cycle.period(), FocusCycle::Period::Focus);
    }

    void tickAtTheEndCountsFocusAndQueuesAShortBreak()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);

        QVERIFY(cycle.tick(FOCUS_MS_));
        QCOMPARE(cycle.state(), FocusCycle::State::Ended);
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.completedFocusCount(), 1);
        QCOMPARE(cycle.remainingMs(FOCUS_MS_), SHORT_MS_);
    }

    void tickReportsAnEndOnce()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);

        QVERIFY(cycle.tick(FOCUS_MS_));
        QVERIFY(!cycle.tick(FOCUS_MS_ + 1));
        QVERIFY(!cycle.tick(FOCUS_MS_ * 10));
    }

    // Asleep through the end, and well past it: the first tick after waking
    // ends the period, and nothing further starts on its own
    void tickLongAfterTheEndEndsOnlyThatPeriod()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);

        QVERIFY(cycle.tick(FOCUS_MS_ * 100));
        QCOMPARE(cycle.state(), FocusCycle::State::Ended);
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.completedFocusCount(), 1);
    }

    void aBreakIsFollowedByFocus()
    {
        auto cycle = makeCycle_(4);
        auto now = runToEnd_(cycle, 0);
        now = runToEnd_(cycle, now);

        QCOMPARE(cycle.state(), FocusCycle::State::Ended);
        QCOMPARE(cycle.period(), FocusCycle::Period::Focus);
        QCOMPARE(cycle.completedFocusCount(), 1);
    }

    void theFocusThatCompletesASetIsFollowedByALongBreak()
    {
        auto cycle = makeCycle_(2);
        auto now = runToEnd_(cycle, 0); // focus 1
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);

        now = runToEnd_(cycle, now); // short break
        now = runToEnd_(cycle, now); // focus 2
        QCOMPARE(cycle.period(), FocusCycle::Period::LongBreak);
        QCOMPARE(cycle.remainingMs(now), LONG_MS_);

        now = runToEnd_(cycle, now); // long break
        now = runToEnd_(cycle, now); // focus 3
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.completedFocusCount(), 3);
    }

    void aSetOfOneIsAlwaysFollowedByALongBreak()
    {
        auto cycle = makeCycle_(1);
        runToEnd_(cycle, 0);

        QCOMPARE(cycle.period(), FocusCycle::Period::LongBreak);
    }

    void longBreakEveryBelowOneIsTakenAsOne()
    {
        auto cycle = makeCycle_(0);
        QCOMPARE(cycle.lengths().longBreakEvery, 1);

        runToEnd_(cycle, 0);
        QCOMPARE(cycle.period(), FocusCycle::Period::LongBreak);
    }

    void pauseHoldsWhatIsLeft()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.pause(300);

        QCOMPARE(cycle.state(), FocusCycle::State::Paused);
        QCOMPARE(cycle.remainingMs(300), FOCUS_MS_ - 300);
        QCOMPARE(cycle.remainingMs(FOCUS_MS_ * 10), FOCUS_MS_ - 300);
        QVERIFY(!cycle.tick(FOCUS_MS_ * 10));
    }

    void resumeGoesOnFromWhatWasLeft()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.pause(300);
        cycle.resume(5'000);

        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QCOMPARE(cycle.remainingMs(5'000), FOCUS_MS_ - 300);
        QVERIFY(!cycle.tick(5'000 + FOCUS_MS_ - 301));
        QVERIFY(cycle.tick(5'000 + FOCUS_MS_ - 300));
    }

    void pauseAfterTheEndLeavesItForTickToEnd()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.pause(FOCUS_MS_ + 5);

        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QVERIFY(cycle.tick(FOCUS_MS_ + 5));
    }

    void pauseAndResumeDoNothingOutOfTurn()
    {
        auto cycle = makeCycle_(4);
        cycle.pause(0);
        QCOMPARE(cycle.state(), FocusCycle::State::Idle);

        cycle.resume(0);
        QCOMPARE(cycle.state(), FocusCycle::State::Idle);

        cycle.start(0);
        cycle.resume(500);
        QCOMPARE(cycle.remainingMs(500), FOCUS_MS_ - 500);
    }

    void startDoesNothingWhileAPeriodIsUnderway()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.start(500);
        QCOMPARE(cycle.remainingMs(500), FOCUS_MS_ - 500);

        cycle.pause(500);
        cycle.start(900);
        QCOMPARE(cycle.state(), FocusCycle::State::Paused);
    }

    void startAfterAnEndRunsTheQueuedPeriod()
    {
        auto cycle = makeCycle_(4);
        auto now = runToEnd_(cycle, 0);
        cycle.start(now);

        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.remainingMs(now), SHORT_MS_);
    }

    void skipWhileRunningStartsTheNextPeriodUncounted()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.skip(300);

        QCOMPARE(cycle.state(), FocusCycle::State::Running);
        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.remainingMs(300), SHORT_MS_);
        QCOMPARE(cycle.completedFocusCount(), 0);
    }

    void skipWhileIdleOrPausedQueuesTheNextPeriod()
    {
        auto idle = makeCycle_(4);
        idle.skip(0);

        QCOMPARE(idle.state(), FocusCycle::State::Idle);
        QCOMPARE(idle.period(), FocusCycle::Period::ShortBreak);

        auto paused = makeCycle_(4);
        paused.start(0);
        paused.pause(300);
        paused.skip(400);

        QCOMPARE(paused.state(), FocusCycle::State::Idle);
        QCOMPARE(paused.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(paused.remainingMs(400), SHORT_MS_);
    }

    // With one focus period counted in a set of two, a completed second one
    // would bring a long break; a skipped one completes nothing
    void aSkippedFocusIsFollowedByAShortBreak()
    {
        auto cycle = makeCycle_(2);
        auto now = runToEnd_(cycle, 0);
        now = runToEnd_(cycle, now);
        cycle.start(now);
        cycle.skip(now);

        QCOMPARE(cycle.period(), FocusCycle::Period::ShortBreak);
        QCOMPARE(cycle.completedFocusCount(), 1);
    }

    void skipAfterAnEndPassesOverTheQueuedPeriod()
    {
        auto cycle = makeCycle_(4);
        auto now = runToEnd_(cycle, 0);
        cycle.skip(now);

        QCOMPARE(cycle.state(), FocusCycle::State::Idle);
        QCOMPARE(cycle.period(), FocusCycle::Period::Focus);
        QCOMPARE(cycle.completedFocusCount(), 1);
    }

    void resetQueuesFocusAndClearsTheCount()
    {
        auto cycle = makeCycle_(4);
        auto now = runToEnd_(cycle, 0);
        cycle.start(now);
        cycle.reset();

        QCOMPARE(cycle.state(), FocusCycle::State::Idle);
        QCOMPARE(cycle.period(), FocusCycle::Period::Focus);
        QCOMPARE(cycle.completedFocusCount(), 0);
        QCOMPARE(cycle.remainingMs(now), FOCUS_MS_);
    }

    void newLengthsApplyFromTheNextPeriodStarted()
    {
        auto cycle = makeCycle_(4);
        cycle.start(0);
        cycle.setLengths({ 7'000, 70, 700, 4 });

        QCOMPARE(cycle.periodLengthMs(), FOCUS_MS_);
        QVERIFY(cycle.tick(FOCUS_MS_));
        QCOMPARE(cycle.remainingMs(FOCUS_MS_), 70);
    }

    void newLengthsShowAtOnceWhileIdle()
    {
        auto cycle = makeCycle_(4);
        cycle.setLengths({ 7'000, 70, 700, 4 });

        QCOMPARE(cycle.remainingMs(0), 7'000);
        QCOMPARE(cycle.periodLengthMs(), 7'000);
    }

    void aZeroLengthPeriodEndsAtOnce()
    {
        auto cycle = FocusCycle({ 0, SHORT_MS_, LONG_MS_, 4 });
        cycle.start(10);

        QVERIFY(cycle.tick(10));
        QCOMPARE(cycle.completedFocusCount(), 1);
    }

private:
    static constexpr qint64 FOCUS_MS_ = 1'000;
    static constexpr qint64 SHORT_MS_ = 200;
    static constexpr qint64 LONG_MS_ = 500;

    static FocusCycle makeCycle_(int longBreakEvery)
    {
        return FocusCycle({ FOCUS_MS_, SHORT_MS_, LONG_MS_, longBreakEvery });
    }

    // Start the queued period at nowMs and tick it to its end. Returns when
    // that was
    static qint64 runToEnd_(FocusCycle& cycle, qint64 nowMs)
    {
        cycle.start(nowMs);
        auto end = nowMs + cycle.periodLengthMs();

        if (!cycle.tick(end)) {
            qWarning("runToEnd_: the period didn't end at its length");
        }

        return end;
    }
};

QTEST_GUILESS_MAIN(FocusCycleTest)

#include "FocusCycleTest.moc"
