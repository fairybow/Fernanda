/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <QList>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "ui/settings/SettingRow.h"
#include "ui/settings/SettingsPage.h"
#include "ui/widgets/DisplaySlider.h"

namespace Fernanda::Ui {

// The settings dialog's Focus timer page: whether this vault's windows show a
// focus timer (core/FocusTimer.h), and how long its periods are. Obsidian has
// no timer of its own, so the page is Fernanda's; its plugins keep their
// settings per vault, as this does.
//
// The lengths hang on the switch, disabled while it's off and keeping their
// values. Each slider applies as it moves, through the Vault, which saves once
// it settles. A new length takes effect from the next period started
class SettingsFocusTimerPage : public SettingsPage
{
    Q_OBJECT

public:
    SettingsFocusTimerPage(Vault* vault, QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
    {
        setup_();
    }

    ~SettingsFocusTimerPage() override { TRACER; }

private:
    void setup_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Focus timer"));

        auto item = addSwitch(
            tr("Show focus timer"),
            tr("Show a timer in the sidebar for working in focus periods, "
               "with a break after each and a longer break after several."),
            config.focusTimerEnabled(),
            &VaultConfig::setFocusTimerEnabled);

        QList<SettingRow*> lengths{};

        lengths << addSlider_(
            tr("Focus"),
            tr("Length of a focus period, in minutes."),
            VaultConfig::MIN_FOCUS_TIMER_FOCUS,
            VaultConfig::MAX_FOCUS_TIMER_FOCUS,
            config.focusTimerFocus(),
            &VaultConfig::setFocusTimerFocus);

        lengths << addSlider_(
            tr("Short break"),
            tr("Length of a short break, in minutes."),
            VaultConfig::MIN_FOCUS_TIMER_BREAK,
            VaultConfig::MAX_FOCUS_TIMER_BREAK,
            config.focusTimerShortBreak(),
            &VaultConfig::setFocusTimerShortBreak);

        lengths << addSlider_(
            tr("Long break"),
            tr("Length of a long break, in minutes."),
            VaultConfig::MIN_FOCUS_TIMER_BREAK,
            VaultConfig::MAX_FOCUS_TIMER_BREAK,
            config.focusTimerLongBreak(),
            &VaultConfig::setFocusTimerLongBreak);

        lengths << addSlider_(
            tr("Long break after"),
            tr("Number of focus periods before each long break."),
            VaultConfig::MIN_FOCUS_TIMER_LONG_BREAK_EVERY,
            VaultConfig::MAX_FOCUS_TIMER_LONG_BREAK_EVERY,
            config.focusTimerLongBreakEvery(),
            &VaultConfig::setFocusTimerLongBreakEvery);

        enableWith(item.toggle, lengths);
    }

    // One number setting: a row holding a slider over [minimum, maximum],
    // seeded with value, each move going to the given VaultConfig setter,
    // through the Vault
    SettingRow* addSlider_(
        const QString& name,
        const QString& description,
        int minimum,
        int maximum,
        int value,
        bool (VaultConfig::*setter)(int))
    {
        auto* slider = new DisplaySlider(minimum, maximum, value, content());

        connect(
            slider,
            &DisplaySlider::valueChanged,
            this,
            [this, setter](int new_value) {
                vault()->setConfig(setter, new_value);
            });

        return addRow(name, description, slider);
    }
};

} // namespace Fernanda::Ui
