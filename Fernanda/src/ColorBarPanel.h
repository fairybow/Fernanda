/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QWidget>

#include "ColorBar.h"
#include "ControlField.h"
#include "Debug.h"
#include "Tr.h"

namespace Fernanda {

class ColorBarPanel : public QWidget
{
    Q_OBJECT

public:
    struct InitialValues
    {
        bool active;
        ColorBar::Position position;
    };

    explicit ColorBarPanel(
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(initialValues);
    }

    virtual ~ColorBarPanel() override { TRACER; }

signals:
    void activeChanged(bool active);
    void positionChanged(ColorBar::Position position);

private:
    QGroupBox* groupBox_ = new QGroupBox(this);
    ControlField<QComboBox*>* position_ =
        new ControlField<QComboBox*>(ControlField<QComboBox*>::Label, this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::colorBarPanelTitle());
        groupBox_->setCheckable(true);
        groupBox_->setChecked(initialValues.active);

        position_->setText(Tr::colorBarPanelPosition());
        auto position_box = position_->control();
        position_box->addItem(Tr::colorBarPanelTop(), ColorBar::Top);
        position_box->addItem(
            Tr::colorBarPanelBelowMenuBar(),
            ColorBar::BelowMenuBar);
        position_box->addItem(
            Tr::colorBarPanelAboveStatusBar(),
            ColorBar::AboveStatusBar);
        position_box->addItem(Tr::colorBarPanelBottom(), ColorBar::Bottom);
        position_box->setCurrentIndex(
            position_box->findData(initialValues.position));

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(position_);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        connect(groupBox_, &QGroupBox::toggled, this, [&](bool toggled) {
            emit activeChanged(toggled);
        });

        connect(
            position_box,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                emit positionChanged(position_->control()
                                         ->itemData(index)
                                         .value<ColorBar::Position>());
            });
    }
};

} // namespace Fernanda
