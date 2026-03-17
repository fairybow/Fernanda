/*
 * Fernanda is a plain text editor for fiction writing
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

#include <QComboBox>
#include <QGroupBox>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "core/Debug.h"
#include "core/Tr.h"
#include "modules/ColorBar.h"
#include "settings/Ini.h"
#include "settings/SettingsPanel.h"
#include "ui/ControlField.h"

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
