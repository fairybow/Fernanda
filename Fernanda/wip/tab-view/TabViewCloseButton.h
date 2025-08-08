#pragma once

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QTabBar>

#include "TabViewButton.h"

class TabViewCloseButton : public TabViewButton
{
    Q_OBJECT

public:
    explicit TabViewCloseButton(QTabBar* tabBar)
        : TabViewButton(tabBar), tabBar_(tabBar)
    {
        initialize_();
    }

    virtual ~TabViewCloseButton() override {}

signals:
    void clickedAt(int index);

private:
    QTabBar* tabBar_;

    void initialize_()
    {
        connect
        (
            this,
            &TabViewCloseButton::clicked,
            this,
            &TabViewCloseButton::onClicked_
        );
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
