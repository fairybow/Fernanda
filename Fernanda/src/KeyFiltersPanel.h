/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QWidget>

#include "ControlInfo.h"
#include "Debug.h"
#include "Tr.h"

namespace Fernanda {

/// TODO KFS
class KeyFiltersPanel : public QWidget
{
    Q_OBJECT

public:
    struct InitialValues
    {
        bool active;
        bool autoClose;
        bool barging;
    };

    explicit KeyFiltersPanel(
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(initialValues);
    }

    virtual ~KeyFiltersPanel() override { TRACER; }

signals:
    void activeChanged(bool active);
    void autoCloseChanged(bool autoClose);
    void bargingChanged(bool barging);

private:
    QGroupBox* groupBox_ = new QGroupBox(this);
    QCheckBox* autoCloseCheck_ = new QCheckBox(this);
    ControlInfo<QCheckBox*>* barging_ = new ControlInfo<QCheckBox*>(this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::keyFiltersPanelTitle());
        groupBox_->setCheckable(true);
        groupBox_->setChecked(initialValues.active);

        autoCloseCheck_->setText(Tr::keyFiltersPanelAutoClose());
        autoCloseCheck_->setChecked(initialValues.autoClose);

        auto barging_check = barging_->control();
        barging_check->setText(Tr::keyFiltersPanelBarging());
        barging_check->setChecked(initialValues.barging);
        barging_->setInfo(Tr::keyFiltersPanelBargingTooltip());

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(autoCloseCheck_);
        group_box_layout->addWidget(barging_);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        connect(groupBox_, &QGroupBox::toggled, this, [&](bool toggled) {
            emit activeChanged(toggled);
        });

        connect(autoCloseCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit autoCloseChanged(toggled);
        });

        connect(barging_check, &QCheckBox::toggled, this, [&](bool toggled) {
            emit bargingChanged(toggled);
        });
    }
};

} // namespace Fernanda
