/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QTabBar>

#include "TabWidgetButton.h"

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
        initialize_();
    }

    virtual ~TabWidgetCloseButton() override {}

signals:
    void clickedAt(int index);

private:
    QTabBar* tabBar_;

    void initialize_()
    {
        connect(
            this,
            &TabWidgetCloseButton::clicked,
            this,
            &TabWidgetCloseButton::onClicked_);
    }

private slots:
    void onClicked_(bool checked)
    {
        (void)checked;

        auto global_center_position = mapToGlobal(rect().center());
        auto tab_bar_position = tabBar_->mapFromGlobal(global_center_position);
        auto tab_index = tabBar_->tabAt(tab_bar_position);

        emit clickedAt(tab_index);
    }
};

} // namespace Fernanda
