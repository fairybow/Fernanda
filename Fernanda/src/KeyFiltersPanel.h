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
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

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
    QCheckBox* bargingCheck_ = new QCheckBox(this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::keyFiltersPanelTitle());
        groupBox_->setCheckable(true);
        groupBox_->setChecked(initialValues.active);

        autoCloseCheck_->setText(Tr::keyFiltersPanelAutoClose());
        autoCloseCheck_->setChecked(initialValues.autoClose);

        bargingCheck_->setText(Tr::keyFiltersPanelBarging());
        bargingCheck_->setChecked(initialValues.barging);

        auto barging_info = new QLabel(this);
        // TODO: This pixmap sucks
        barging_info->setPixmap(
            style()
                ->standardPixmap(QStyle::SP_MessageBoxInformation)
                .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        barging_info->setToolTip(Tr::keyFiltersPanelBargingTooltip());

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto barging_layout = new QHBoxLayout;
        barging_layout->addWidget(bargingCheck_);
        barging_layout->addWidget(barging_info);
        barging_layout->addStretch();

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(autoCloseCheck_);
        group_box_layout->addLayout(barging_layout);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        connect(groupBox_, &QGroupBox::toggled, this, [&](bool toggled) {
            emit activeChanged(toggled);
        });

        connect(autoCloseCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit autoCloseChanged(toggled);
        });

        connect(bargingCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit bargingChanged(toggled);
        });
    }
};

} // namespace Fernanda
