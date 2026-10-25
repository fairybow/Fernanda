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

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QHelpEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QtNumeric>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/FocusCycle.h"
#include "core/FocusTimer.h"
#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The focus timer's button, left of the gear in the sidebar's footer: its
// window's FocusTimer, shown and driven. The Sidebar shows it while the
// vault's timer is switched on.
//
// The glyph names the period running or queued: Timer for focus, Coffee for a
// break. A ring around it shows how much of the period is left, emptying as it
// runs: in the accent while it runs, muted while paused, and full in the
// accent once a period has run out and the next waits to be started. Idle, the
// ring is only its faint track. The exact time is in the tooltip and at the
// top of the right-click menu, so the button itself never shows a ticking
// number.
//
// A click starts the queued period, pauses a running one, or resumes a paused
// one. The right-click menu has the same, plus skipping to the next period and
// resetting. When a period runs out, the button flashes its window
// (QApplication::alert), which does nothing while the window is active.
//
// It talks to the timer directly: the window never reaches into it, and
// nothing is relayed
class FocusTimerButton : public GlyphButton
{
    Q_OBJECT

public:
    FocusTimerButton(FocusTimer* focusTimer, QWidget* parentSidebar)
        : GlyphButton(
              FOCUS_TIMER_BUTTON_EXTENT,
              FOCUS_TIMER_BUTTON_ICON_EXTENT,
              parentSidebar)
        , focusTimer_(focusTimer)
    {
        setup_();
    }

    ~FocusTimerButton() override { TRACER; }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        auto is_focus = cycle_().period() == FocusCycle::Period::Focus;

        return QString::fromLatin1(
            is_focus ? FOCUS_TIMER_FOCUS_ICON_PATH
                     : FOCUS_TIMER_BREAK_ICON_PATH);
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return FOCUS_TIMER_ICON_ROLE;
    }

    bool event(QEvent* event) override
    {
        // The tooltip is written when it opens, since what is left changes
        // every second and a Qt tooltip doesn't update while it's open
        if (event->type() == QEvent::ToolTip) {
            auto* help = static_cast<QHelpEvent*>(event);
            QToolTip::showText(
                help->globalPos(),
                statusText_() + u"\n"_s + countText_(),
                this);
            return true;
        }

        return GlyphButton::event(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        GlyphButton::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        auto inset = FOCUS_TIMER_RING_INSET + FOCUS_TIMER_RING_WIDTH / 2.0;
        auto ring = QRectF(rect()).adjusted(inset, inset, -inset, -inset);

        QPen pen(Style::divider(palette()), FOCUS_TIMER_RING_WIDTH);
        pen.setCapStyle(Qt::FlatCap);
        painter.setPen(pen);
        painter.drawEllipse(ring);

        auto fraction = ringFraction_();
        if (fraction <= 0.0) {
            return;
        }

        auto paused = cycle_().state() == FocusCycle::State::Paused;
        pen.setColor(
            palette().color(
                paused ? FOCUS_TIMER_PAUSED_RING_ROLE : FOCUS_TIMER_RING_ROLE));
        pen.setCapStyle(fraction < 1.0 ? Qt::RoundCap : Qt::FlatCap);
        painter.setPen(pen);

        // Qt's angles are sixteenths of a degree, counterclockwise from three
        // o'clock. From twelve o'clock, a negative span runs clockwise, so the
        // arc left shrinks back toward twelve as the period runs
        constexpr auto TWELVE_O_CLOCK = 90 * 16;
        constexpr auto FULL_CIRCLE = 360 * 16;
        painter.drawArc(ring, TWELVE_O_CLOCK, -qRound(fraction * FULL_CIRCLE));
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        QMenu menu(this);

        auto* status = menu.addAction(statusText_());
        status->setEnabled(false);
        auto* count = menu.addAction(countText_());
        count->setEnabled(false);

        menu.addSeparator();

        connect(
            menu.addAction(clickActionText_()),
            &QAction::triggered,
            this,
            &FocusTimerButton::onClicked_);

        auto skip_text = cycle_().period() == FocusCycle::Period::Focus
                             ? tr("Skip to break")
                             : tr("Skip to focus");

        connect(
            menu.addAction(skip_text),
            &QAction::triggered,
            focusTimer_,
            &FocusTimer::skip);

        auto* reset = menu.addAction(tr("Reset"));
        reset->setEnabled(!isAtStart_());
        connect(reset, &QAction::triggered, focusTimer_, &FocusTimer::reset);

        menu.exec(event->globalPos());
    }

private:
    // The window's timer, which owns this button's subject and outlives it:
    // both are the window's, and the timer is a direct child of it
    FocusTimer* focusTimer_ = nullptr;

    void setup_()
    {
        connect(
            focusTimer_,
            &FocusTimer::changed,
            this,
            qOverload<>(&QWidget::update));

        connect(focusTimer_, &FocusTimer::periodEnded, this, [this] {
            QApplication::alert(window());
        });

        connect(
            this,
            &FocusTimerButton::clicked,
            this,
            &FocusTimerButton::onClicked_);
    }

    [[nodiscard]] const FocusCycle& cycle_() const
    {
        return focusTimer_->cycle();
    }

    // Start the queued period, pause a running one, or resume a paused one
    void onClicked_()
    {
        switch (cycle_().state()) {
        case FocusCycle::State::Running:
            focusTimer_->pause();
            break;
        case FocusCycle::State::Paused:
            focusTimer_->resume();
            break;
        case FocusCycle::State::Idle:
        case FocusCycle::State::Ended:
        default:
            focusTimer_->start();
            break;
        }
    }

    // What a click does now, as a menu item
    [[nodiscard]] QString clickActionText_() const
    {
        switch (cycle_().state()) {
        case FocusCycle::State::Running:
            return tr("Pause");
        case FocusCycle::State::Paused:
            return tr("Resume");
        case FocusCycle::State::Idle:
        case FocusCycle::State::Ended:
        default:
            return tr("Start");
        }
    }

    // Nothing to reset: focus queued, not started, nothing counted
    [[nodiscard]] bool isAtStart_() const
    {
        return cycle_().state() == FocusCycle::State::Idle &&
               cycle_().period() == FocusCycle::Period::Focus &&
               cycle_().completedFocusCount() == 0;
    }

    // How much of the ring to draw: what is left of a period running or
    // paused, all of it once a period has run out, and none while idle
    [[nodiscard]] double ringFraction_() const
    {
        switch (cycle_().state()) {
        case FocusCycle::State::Running:
        case FocusCycle::State::Paused: {
            auto length = cycle_().periodLengthMs();
            if (length <= 0) {
                return 0.0;
            }

            auto left = cycle_().remainingMs(Coco::Time::epoch());
            return static_cast<double>(left) / static_cast<double>(length);
        }
        case FocusCycle::State::Ended:
            return 1.0;
        case FocusCycle::State::Idle:
        default:
            return 0.0;
        }
    }

    [[nodiscard]] QString periodName_() const
    {
        switch (cycle_().period()) {
        case FocusCycle::Period::ShortBreak:
            return tr("Short break");
        case FocusCycle::Period::LongBreak:
            return tr("Long break");
        case FocusCycle::Period::Focus:
        default:
            return tr("Focus");
        }
    }

    // The period and its time, rounded up as a countdown reads
    [[nodiscard]] QString statusText_() const
    {
        auto left = Coco::Time::clockQString(
            cycle_().remainingMs(Coco::Time::epoch()),
            Coco::Time::RoundUp::Yes);

        switch (cycle_().state()) {
        case FocusCycle::State::Running:
            return tr("%1, %2 left").arg(periodName_(), left);
        case FocusCycle::State::Paused:
            return tr("%1, paused with %2 left").arg(periodName_(), left);
        case FocusCycle::State::Ended:
            return tr("Time's up. Next: %1, %2").arg(periodName_(), left);
        case FocusCycle::State::Idle:
        default:
            return tr("%1, %2, ready to start").arg(periodName_(), left);
        }
    }

    [[nodiscard]] QString countText_() const
    {
        return tr(
            "%Ln focus period(s) completed",
            nullptr,
            cycle_().completedFocusCount());
    }
};

} // namespace Fernanda::Ui
