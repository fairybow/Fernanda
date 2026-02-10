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

#include "Debug.h"
#include "Tr.h"

namespace Fernanda {

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
    //

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

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(autoCloseCheck_);
        group_box_layout->addWidget(bargingCheck_);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        //...
    }
};

} // namespace Fernanda
