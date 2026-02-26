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
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "ColorBar.h"
#include "ControlField.h"
#include "Debug.h"
#include "Ini.h"
#include "SettingsPanel.h"
#include "Tr.h"

namespace Fernanda {

class ColorBarPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit ColorBarPanel(const QVariantMap& values, QWidget* parent = nullptr)
        : SettingsPanel(Tr::colorBarPanelTitle(), parent)
    {
        setup_(values);
    }

    virtual ~ColorBarPanel() override { TRACER; }

private:
    ControlField<QComboBox>* position_ =
        new ControlField<QComboBox>(FieldKind::Label, this);

    void setup_(const QVariantMap& values)
    {
        // Populate
        auto group_box = groupBox();
        group_box->setCheckable(true);
        group_box->setChecked(values[Ini::Keys::COLOR_BAR_ACTIVE].toBool());

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
        position_box->setCurrentIndex(position_box->findData(
            values[Ini::Keys::COLOR_BAR_POSITION].value<ColorBar::Position>()));

        // Layout
        group_box->layout()->addWidget(position_);

        // Connect
        connectGroupBox(Ini::Keys::COLOR_BAR_ACTIVE);
        connectComboBox(position_box, Ini::Keys::COLOR_BAR_POSITION);
    }
};

} // namespace Fernanda
