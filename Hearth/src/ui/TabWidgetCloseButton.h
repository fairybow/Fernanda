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

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QTabBar>

#include "ui/TabWidgetButton.h"

namespace Fernanda {

// Index-"aware" tab close button
class TabWidgetCloseButton : public TabWidgetButton
{
    Q_OBJECT

public:
    explicit TabWidgetCloseButton(QTabBar* tabBar)
        : TabWidgetButton(tabBar)
        , tabBar_(tabBar)
    {
        setup_();
    }

signals:
    void clickedAt(int index);

private:
    QTabBar* tabBar_;

    void setup_()
    {
        connect(
            this,
            &TabWidgetCloseButton::clicked,
            this,
            &TabWidgetCloseButton::onClicked_);
    }

private slots:
    void onClicked_([[maybe_unused]] bool checked)
    {
        auto global_center_position = mapToGlobal(rect().center());
        auto tab_bar_position = tabBar_->mapFromGlobal(global_center_position);
        auto tab_index = tabBar_->tabAt(tab_bar_position);

        emit clickedAt(tab_index);
    }
};

} // namespace Fernanda
