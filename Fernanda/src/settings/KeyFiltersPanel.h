/*
 * Fernanda — a plain-text-first workbench for creative writing
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

#include <QCheckBox>
#include <QGroupBox>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "core/Debug.h"
#include "core/Tr.h"
#include "settings/Ini.h"
#include "settings/SettingsPanel.h"
#include "ui/ControlField.h"

namespace Fernanda {

class KeyFiltersPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit KeyFiltersPanel(
        const QVariantMap& values,
        QWidget* parent = nullptr)
        : SettingsPanel(Tr::keyFiltersPanelTitle(), parent)
    {
        setup_(values);
    }

    virtual ~KeyFiltersPanel() override { TRACER; }

private:
    QCheckBox* autoCloseCheck_ = new QCheckBox(this);
    ControlField<QCheckBox>* barging_ =
        new ControlField<QCheckBox>(FieldKind::Info, this);

    void setup_(const QVariantMap& values)
    {
        // Populate
        auto group_box = groupBox();
        group_box->setCheckable(true);
        group_box->setChecked(values[Ini::Keys::KEY_FILTERS_ACTIVE].toBool());

        autoCloseCheck_->setText(Tr::keyFiltersPanelAutoClose());
        autoCloseCheck_->setChecked(
            values[Ini::Keys::KEY_FILTERS_AUTO_CLOSE].toBool());

        auto barging_check = barging_->control();
        barging_check->setText(Tr::keyFiltersPanelBarging());
        barging_check->setChecked(
            values[Ini::Keys::KEY_FILTERS_BARGING].toBool());
        barging_->setInfo(Tr::keyFiltersPanelBargingTooltip());

        // Layout
        auto layout = group_box->layout();
        layout->addWidget(autoCloseCheck_);
        layout->addWidget(barging_);

        // Connect
        connectGroupBox(Ini::Keys::KEY_FILTERS_ACTIVE);
        connectCheckBox(autoCloseCheck_, Ini::Keys::KEY_FILTERS_AUTO_CLOSE);
        connectCheckBox(barging_check, Ini::Keys::KEY_FILTERS_BARGING);
    }
};

} // namespace Fernanda
